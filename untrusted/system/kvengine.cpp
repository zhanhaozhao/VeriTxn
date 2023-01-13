#include <iostream>
#include <thread>
#include "kvengine.h"

#if USE_SGX != 1

using namespace rocksdb;

std::string kDBPath = "./storage/rocksdb";

static const size_t kDefaultWriteBufferSize(128 << 20);
static const uint64_t kDefaultMemtableMemoryBudget(512 << 20);
// static constexpr char backup_dir_suffix[8] = "_backup";

static const size_t kCacheSizeBytes(128 << 20);
static const size_t kWriteBufferSize(256 << 20);
static const uint64_t kMemtableMemoryBudget(1 << 30);

int kvengine::testkv(){
// int testkv(){
  DB* db;
  Options options;
  // Optimize kvengine. This is the easiest way to get kvengine to perform well
  options.IncreaseParallelism();
//   options.write_buffer_size
  options.OptimizeLevelStyleCompaction();
  // create the DB if it's not already present
  options.create_if_missing = true;

  // open DB
  Status s = DB::Open(options, kDBPath, &db);
  std::cout<<"status"<<s.getState()<<std::endl;
  assert(s.ok());

  // Put key-value
  s = db->Put(WriteOptions(), "key1", "value");
  assert(s.ok());
  std::string value;
  // get value
  s = db->Get(ReadOptions(), "key1", &value);
  assert(s.ok());
  assert(value == "value");

  // atomically apply a set of updates
  {
    WriteBatch batch;
    batch.Delete("key1");
    batch.Put("key2", value);
    s = db->Write(WriteOptions(), &batch);
  }

  s = db->Get(ReadOptions(), "key1", &value);
  assert(s.IsNotFound());

  db->Get(ReadOptions(), "key2", &value);
  assert(value == "value");

  {
    PinnableSlice pinnable_val;
    db->Get(ReadOptions(), db->DefaultColumnFamily(), "key2", &pinnable_val);
    assert(pinnable_val == "value");
  }

  {
    std::string string_val;
    // If it cannot pin the value, it copies the value to its internal buffer.
    // The intenral buffer could be set during construction.
    PinnableSlice pinnable_val(&string_val);
    db->Get(ReadOptions(), db->DefaultColumnFamily(), "key2", &pinnable_val);
    assert(pinnable_val == "value");
    // If the value is not pinned, the internal buffer must have the value.
    assert(pinnable_val.IsPinned() || string_val == "value");
  }

  PinnableSlice pinnable_val;
  db->Get(ReadOptions(), db->DefaultColumnFamily(), "key1", &pinnable_val);
  assert(s.IsNotFound());
  // Reset PinnableSlice after each use and before each reuse
  pinnable_val.Reset();
  db->Get(ReadOptions(), db->DefaultColumnFamily(), "key2", &pinnable_val);
  assert(pinnable_val == "value");
  pinnable_val.Reset();
  // The Slice pointed by pinnable_val is not valid after this point

  delete db;

  return 0;
}

kvengine::kvengine()
  : db_(nullptr) {
    db_opts_.create_if_missing = true;
    db_opts_.write_buffer_size = kDefaultWriteBufferSize;
    db_opts_.IncreaseParallelism(std::thread::hardware_concurrency());
    db_opts_.OptimizeLevelStyleCompaction(kDefaultMemtableMemoryBudget);
}

bool kvengine::OpenDB(const std::string& db_path) {
//   std::string backup_path{db_path+backup_dir_suffix};
  
    db_opts_.error_if_exists = false;
    db_opts_.OptimizeLevelStyleCompaction(kMemtableMemoryBudget);
    db_opts_.write_buffer_size = kWriteBufferSize;
  
  if (db_ != nullptr) {
    // LOG(ERROR) << "DB is already opened";
    std::cout << "DB is already opened" << std::endl;
    return false;
  }

  // Note: NewPrefixTransform() shouldn't be called in the constructor.
//   const rocksdb::SliceTransform* prefix_trans = NewPrefixTransform();
//   if (prefix_trans != nullptr) {
    // db_opts_.prefix_extractor.reset(prefix_trans);
    // db_read_opts_.prefix_same_as_start = true;
//   }
  // Note: NewMergeOperator() shouldn't be called in the constructor.
//   rocksdb::MergeOperator* merge_op = NewMergeOperator();
//   if (merge_op != nullptr) db_opts_.merge_operator.reset(merge_op);

//   db_opts_.table_factory.reset(
    // rocksdb::NewBlockBasedTableFactory(db_blk_tab_opts_));

  auto db_stat = rocksdb::DB::Open(db_opts_, db_path, &db_);
  if (!db_stat.ok()) {
    std::cout << "Failed to open DB: " << db_stat.ToString() << std::endl;
    return false;
  }
  db_path_ = db_path;

  // initialize backup engine
//   db_stat = rocksdb::BackupEngine::Open(rocksdb::Env::Default(), 
    // rocksdb::BackupableDBOptions(backup_path), &backup_engine_);
//   if (!db_stat.ok()) {
    // LOG(ERROR) << "Failed to initialize backup engine: " << db_stat.ToString();
    // return false;
//   }
  std::cout << "DB is successfully opened: " << db_path_ << std::endl;
  return true;
}

void kvengine::CloseDB(const bool flush) {
  if (flush) FlushDB();
  if (db_ != nullptr) delete db_;
  // keep only the latest backup.
//   if (backup_engine_) {
//     auto s = backup_engine_->PurgeOldBackups(1);
//     if (!s.ok()) {
//       LOG(ERROR) << "Failed to clean up old backups";
//     }
//     delete backup_engine_;
//   }
  db_ = nullptr;
//   backup_engine_ = nullptr;
}

bool kvengine::DestroyDB(const std::string& db_path) {
  auto db_stat = rocksdb::DestroyDB(db_path, rocksdb::Options());
  // remove backup directory.
//   std::string backup_path{db_path + backup_dir_suffix};
//   rocksdb::Env::Default()->DeleteDir(backup_path); 
  if (db_stat.ok()) {
    std::cout << "DB at \"" << db_path << "\" has been deleted" << std::endl;
    return true;
  } else {
    std::cout << "Failed to delete DB: " << db_path << std::endl;
    return false;
  }
}

bool kvengine::DestroyDB() {
  CloseDB(false);
  bool success = db_path_.empty() ? false : DestroyDB(db_path_);
  if (success) db_path_.clear();
  return success;
}

bool kvengine::FlushDB() {
  auto db_stat = db_->Flush(db_flush_opts_);
  bool success = db_stat.ok();
  assert(success);
//    << "Failed to flush DB: " << db_stat.ToString();
  return success;
}

void kvengine::DBFullScan(
  const std::function<void(const rocksdb::Iterator*)>& f_proc_entry) const {
  auto it = db_->NewIterator(db_read_opts_);
  for (it->SeekToFirst(); it->Valid(); it->Next()) f_proc_entry(it);
  delete it;
}

void kvengine::DBPrefixScan(
  const rocksdb::Slice& seek_key,
  const std::function<void(const rocksdb::Iterator*)>& f_proc_entry) const {
  auto it = db_->NewIterator(db_read_opts_);
  for (it->Seek(seek_key); it->Valid(); it->Next()) f_proc_entry(it);
  delete it;
}

//void kvengine::DBDeletePrefix(
//        const rocksdb::Slice& start_key,
//        const rocksdb::Slice& end_key) const {
//    db_->DeleteRange(WriteOptions(), 0, start_key, end_key);
//}

bool kvengine::BackupDB() {
//   if (backup_engine_ == nullptr) {
//     LOG(ERROR) << "Backup engine does not exist";
//     return false;
//   }
//   auto s = backup_engine_->CreateNewBackup(db_);
//   if (s.ok()) return true;
//   std::cout << "Backup creation failed: " << s.ToString() << std::endl;
  return false;
}

bool kvengine::CheckoutBackupDB(const std::string& dst_dir) {
//   if (backup_engine_ == nullptr) {
//     LOG(ERROR) << "Backup engine does not exist";
//     return false;
//   }
//   auto s = backup_engine_->RestoreDBFromLatestBackup(dst_dir, dst_dir);
//   if (s.ok()) return true;
//   std::cout << "Backup Checkout failed: " << s.ToString() << std::endl;
  return false;
}

#endif