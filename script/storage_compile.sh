# set -x

pwd
cd ../untrusted/system/
#protoc --cpp_out=. --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` storage.proto

cd ../../common/
pwd
sed -i '123c #define LOG_RECOVER                 true' config.h
sed -i '251c #define USE_SGX 0' config.h

cd ../
pwd
cp sgx_files/Makefile.storage Makefile
make clean && make -j10

### cleanup
make no-sgx
cd common/
pwd
sed -i '123c #define LOG_RECOVER                 false' config.h
sed -i '251c #define USE_SGX 0' config.h