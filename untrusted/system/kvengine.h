#ifndef KVENGINE_H
#define KVENGINE_H

#include "config.h"

#if USE_SGX != 1

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"

class kvengine {

public:
	int testkv();

public:
//   static inline rocksdb::Slice ToRocksSlice(const ustore::Slice& x) {
//     return rocksdb::Slice(reinterpret_cast<const char*>(x.data()), x.len());
//   }

//   static inline rocksdb::Slice ToRocksSlice(const ustore::Hash& x) {
//     return rocksdb::Slice(reinterpret_cast<const char*>(x.value()),
//                           ustore::Hash::kByteLength);
//   }

//   static inline rocksdb::Slice ToRocksSlice(const ustore::Chunk& x) {
//     return rocksdb::Slice(reinterpret_cast<const char*>(x.head()),
//                           x.numBytes());
//   }

//   static inline ustore::Chunk ToChunk(const rocksdb::Slice& x) {
//     const auto data_size = x.size();
//     std::unique_ptr<byte_t[]> buf(new byte_t[data_size]);
//     std::memcpy(buf.get(), x.data(), data_size);
//     return Chunk(std::move(buf));
//   }

  static inline rocksdb::Env* DefaultEnv() { return rocksdb::Env::Default(); }

  kvengine();
  ~kvengine() = default;

  bool OpenDB(const std::string& db_path);

  void CloseDB(const bool flush = true);

  static bool DestroyDB(const std::string& db_path);

  bool DestroyDB();

  bool FlushDB();

  bool BackupDB();

  bool CheckoutBackupDB(const std::string& dst_dir);

//  protected:
  virtual const rocksdb::SliceTransform* NewPrefixTransform() const {
    return nullptr;
  }

  virtual rocksdb::MergeOperator* NewMergeOperator() const {
    return nullptr;
  }

  inline bool DBGet(const rocksdb::Slice& key, std::string* value) const {
    return db_->Get(db_read_opts_, key, value).ok();
  }

  inline bool DBGet(const rocksdb::Slice& key,
                    rocksdb::PinnableSlice* value) const {
    return db_->Get(db_read_opts_, db_->DefaultColumnFamily(), key, value).ok();
  }

  inline bool DBPut(const rocksdb::Slice& key, const rocksdb::Slice& value) {
    return db_->Put(db_write_opts_, key, value).ok();
  }

  inline bool DBDelete(const rocksdb::Slice& key) {
    return db_->Delete(db_write_opts_, key).ok();
  }

  inline bool DBMerge(const rocksdb::Slice& key, const rocksdb::Slice& value) {
    return db_->Merge(db_write_opts_, key, value).ok();
  }

  inline bool DBWrite(rocksdb::WriteBatch* updates) {
    return db_->Write(db_write_opts_, updates).ok();
  }

  inline bool DBExists(const rocksdb::Slice& key) const {
    rocksdb::PinnableSlice pin_val;
    return DBGet(key, &pin_val);
  }

    inline void DBDeletePrefix(const rocksdb::Slice &start_key, const rocksdb::Slice &end_key) const;
    void DBFullScan(
    const std::function<void(const rocksdb::Iterator*)>& f_proc_entry) const;

  void DBPrefixScan(
    const rocksdb::Slice& seek_key,
    const std::function<void(const rocksdb::Iterator*)>& f_proc_entry) const;

  std::string db_path_;
  rocksdb::DB* db_;
  rocksdb::Options db_opts_;
  rocksdb::ReadOptions db_read_opts_;
  rocksdb::WriteOptions db_write_opts_;
  rocksdb::FlushOptions db_flush_opts_;

};

#endif

#endif
