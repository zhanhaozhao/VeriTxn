SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= HW
SGX_ARCH ?= x64
SGX_DEBUG ?= 0

PROJECT_ROOT_DIR := $(shell readlink -f ..)
# Enclave_Search_Dirs ?= $(shell \
# 	find $(INFERENCE_RT_DIR)/include -maxdepth 1 -type d -not -path '*/.' -printf '--search-path %p '\
# )

INSTALL ?= install
INSTALL_PREFIX ?= ./install
INSTALL_LIB_DIR = $(INSTALL_PREFIX)/lib
INSTALL_INCLUDE_DIR = $(INSTALL_PREFIX)/include

.PHONY: all install clean mrproper

SRC_DIRS = ./ ./common/ ./untrusted/system/ ./untrusted/benchmarks/ ./untrusted/cache/ ./trusted/system/ ./trusted/concurrency_control/
CPPS = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)*.cpp))
OBJS = $(CPPS:.cpp=.o)
DEPS = $(CPPS:.cpp=.d)

all:
	$(MAKE) -ef sgx_t.mk all SGX_MODE=$(SGX_MODE) SGX_DEBUG=$(SGX_DEBUG) Enclave_Search_Dirs="$(Enclave_Search_Dirs)"
	$(MAKE) -ef sgx_u.mk all SGX_MODE=$(SGX_MODE) SGX_DEBUG=$(SGX_DEBUG) Enclave_Search_Dirs="$(Enclave_Search_Dirs)"

HEADERS := untrusted/worker.h

install:
	$(INSTALL) -d $(INSTALL_INCLUDE_DIR)
	$(INSTALL) -d $(INSTALL_LIB_DIR)
	# $(INSTALL) -C -m 644 ${HEADERS} $(INSTALL_INCLUDE_DIR)
	# $(INSTALL) -C -m 664 *.signed.so $(INSTALL_LIB_DIR)
	# $(INSTALL) -C -m 644 *.a $(INSTALL_LIB_DIR)

clean:
	$(MAKE) -ef sgx_t.mk clean
	$(MAKE) -ef sgx_u.mk clean
	rm -f rundb ./trusted/*.o ./trusted/*.d ./untrusted/*.o ./untrusted/*.d ./common/*.o ./common/*.d $(OBJS) $(DEPS)

mrproper: clean
	rm -rf ./install

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