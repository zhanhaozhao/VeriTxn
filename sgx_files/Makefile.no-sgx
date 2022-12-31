PROJECT_ROOT ?= $(shell readlink -f .)


CC=g++
CFLAGS=-w -g -std=c++0x -no-pie

.SUFFIXES: .o .cc .cpp .h

SRC_DIRS = ./ ./common/ ./untrusted/system/ ./untrusted/benchmarks/ ./untrusted/cache/ ./trusted/system/ ./trusted/concurrency_control/
INCLUDE = -I. -I./common/ -I./untrusted/system/ -I./untrusted/benchmarks/ -I./untrusted/cache/ -I./trusted/system/ -I./trusted/concurrency_control/

# SRC_DIRS = ./ ./common ./untrusted/system/ ./untrusted/benchmarks/ ./untrusted/cache/
# INCLUDE = -I. -I./common -I./untrusted/system -I./untrusted/benchmarks/ -I./untrusted/cache/

# CFLAGS += $(INCLUDE) -D NOGRAPHITE=1 -no-pie -O0
# LDFLAGS = -Wall -L. -L./libs -pthread -g -lrt -std=c++0x -O0 -ljemalloc

# protoc --cpp_out=. --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` storage.proto
CFLAGS += $(INCLUDE) -D NOGRAPHITE=1 -Werror -Wno-comment -O0 `pkg-config --cflags protobuf grpc`
LDFLAGS = -Wall -L.  -L./libs -pthread -g -lrt -std=c++0x -O0 -ljemalloc -lnanomsg  -lrocksdb `pkg-config --libs protobuf grpc++ grpc`
# LDFLAGS = -Wall -L. -pthread -g -lrt -std=c++0x -O0 -ljemalloc -fsanitize=address -fno-omit-frame-pointer -static-libasan
LDFLAGS += $(CFLAGS)

CPPS = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)*.cpp))
CCS = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)*.cc))
CCOBJS = $(CCS:.cc=.o)
OBJS = $(CPPS:.cpp=.o)
DEPS = $(CPPS:.cpp=.d)

all:App

App : $(CCOBJS) $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

-include $(CCOBJS:%.o=%.d) $(OBJS:%.o=%.d)

%.d: %.cc
	$(CC) -MM -MT $*.o -MF $@ $(CFLAGS) $<

%.d: %.cpp
	$(CC) -MM -MT $*.o -MF $@ $(CFLAGS) $<

%.o: %.cc
	$(CC) -c $(CFLAGS) -o $@ $<

%.o: %.cpp
	$(CC) -c $(CFLAGS) -o $@ $<

clean:
	rm -f App ./trusted/*.o ./trusted/*.d ./trusted/system/*.o ./trusted/system/*.d ./trusted/concurrency_control/*.o trusted/concurrency_control/*.d ./untrusted/*.o ./untrusted/*.d ./untrusted/system/*.o ./untrusted/system/*.d ./untrusted/benchmarks/*.o ./untrusted/benchmarks/*.d ./untrusted/cache/*.o ./untrusted/cache/*.d ./common/*.o ./common/*.d $(OBJS) $(DEPS)

no-sgx:
	cp ./sgx_files/Makefile.no-sgx Makefile

sgx-release:
	cp ./sgx_files/release/main.cpp ./main.cpp
	cp ./sgx_files/release/Makefile.sgx ./sgx_files/Makefile.sgx
	cp ./sgx_files/release/sgx_t.mk ./sgx_t.mk
	cp ./sgx_files/release/sgx_u.mk ./sgx_u.mk
	cp ./sgx_files/release/Enclave.config.xml ./trusted/Enclave.config.xml
	cp ./sgx_files/Makefile.sgx Makefile

sgx-debug:
	cp ./sgx_files/debug/main.cpp ./main.cpp
	cp ./sgx_files/debug/Makefile.sgx ./sgx_files/Makefile.sgx
	cp ./sgx_files/debug/sgx_t.mk ./sgx_t.mk
	cp ./sgx_files/debug/sgx_u.mk ./sgx_u.mk
	cp ./sgx_files/debug/Enclave.config.xml ./trusted/Enclave.config.xml
	cp ./sgx_files/Makefile.sgx Makefile

exp:
	cd ./untrusted/system && protoc --cpp_out=. --grpc_out=. --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` storage.proto
	cd ./script/ && pwd && python ./run_experiments.py -e -r -ns -c vcloud ycsb_debug
