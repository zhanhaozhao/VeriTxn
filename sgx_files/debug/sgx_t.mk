### Project Settings ###
PROJECT_ROOT_DIR ?= $(shell readlink -f .)
### Intel(R) SGX SDK Settings ###
SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= HW
SGX_ARCH ?= x64
SGX_DEBUG ?= 1
ifeq ($(shell getconf LONG_BIT), 32)
	SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
	SGX_ARCH := x86
endif

ifeq ($(SGX_ARCH), x86)
	SGX_COMMON_CFLAGS := -m32
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x86/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x86/sgx_edger8r
else
	SGX_COMMON_CFLAGS := -m64
	SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
	SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x64/sgx_sign
	SGX_EDGER8R := $(SGX_SDK)/bin/x64/sgx_edger8r
endif

ifeq ($(SGX_DEBUG), 1)
ifeq ($(SGX_PRERELEASE), 1)
$(error Cannot set SGX_DEBUG and SGX_PRERELEASE at the same time!!)
endif
endif

ifeq ($(SGX_DEBUG), 1)
        SGX_COMMON_CFLAGS += -O0 -g -DSGX_DEBUG
else
        SGX_COMMON_CFLAGS += -O2
endif

ifneq ($(SGX_MODE), HW)
	Trts_Library_Name := sgx_trts_sim
	Service_Library_Name := sgx_tservice_sim
else
	Trts_Library_Name := sgx_trts
	Service_Library_Name := sgx_tservice
endif

ifeq ($(SGX_MODE), HW)
ifneq ($(SGX_DEBUG), 1)
ifneq ($(SGX_PRERELEASE), 1)
Build_Mode = HW_RELEASE
endif
endif
endif

SGX_COMMON_FLAGS += -Wall -Wextra -Wchar-subscripts -Wno-coverage-mismatch \
										-Winit-self -Wpointer-arith -Wreturn-type \
                    -Waddress -Wsequence-point -Wformat-security \
                    -Wmissing-include-dirs -Wfloat-equal -Wundef -Wshadow \
                    -Wcast-align -Wcast-qual -Wconversion -Wredundant-decls
### Intel(R) SGX SDK Settings ###

### Project Settings ###
SGX_Include_Paths := -I$(SGX_SDK)/include -I$(SGX_SDK)/include/tlibc \
						 -I$(SGX_SDK)/include/libcxx

Flags_Just_For_C := -Wno-implicit-function-declaration -std=c11 -D_GLIBCXX_USE_CXX11_ABI=0
Flags_Just_For_Cpp := -Wnon-virtual-dtor -std=c++11 -nostdinc++ -D_GLIBCXX_USE_CXX11_ABI=0
Common_C_Cpp_Flags := $(SGX_COMMON_CFLAGS) $(SGX_COMMON_FLAGS) -nostdinc -fvisibility=hidden -fpie -fstack-protector -fno-builtin -fno-builtin-printf -I.
Common_C_Flags := -Wjump-misses-init -Wstrict-prototypes \
										-Wunsuffixed-float-constants

Enclave_C_Flags := $(Flags_Just_For_C) $(Common_C_Cpp_Flags) $(Common_C_Flags) -Itrusted $(SGX_Include_Paths)
Enclave_C_Flags += -I$(PROJECT_ROOT_DIR) -Icommon/ -Itrusted/ -Itrusted/concurrency_control -Itrusted/system -D NOGRAPHITE=1

Enclave_Cpp_Flags := $(Flags_Just_For_Cpp) $(Common_C_Cpp_Flags) $(SGX_RA_TLS_Extra_Flags) -DSGXCLIENT -Itrusted $(SGX_Include_Paths)
Enclave_Cpp_Flags += -I$(PROJECT_ROOT_DIR) -Icommon/ -Itrusted/ -Itrusted/concurrency_control -Itrusted/system -D NOGRAPHITE=1

Crypto_Library_Name := sgx_tcrypto

# -L$(PROJECT_ROOT_DIR)/libs -l:libjemalloc.a
Enclave_Link_Flags := $(SGX_COMMON_CFLAGS) \
	-Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles \
	-L$(SGX_LIBRARY_PATH) \
	-Wl,--whole-archive -l$(Trts_Library_Name) -Wl,--no-whole-archive \
	-Wl,--start-group -lsgx_tstdc -lsgx_tcxx -lsgx_pthread -l$(Crypto_Library_Name) \
	-l$(Service_Library_Name) -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic \
	-Wl,--defsym,__ImageBase=0 \
	-Wl,--version-script=trusted/Enclave.lds

### Project Settings ###

### Phony targets ###
.PHONY: all clean

### Build all ###
ifeq ($(Build_Mode), HW_RELEASE)
all: Enclave.so
	@echo "Build enclave Server_Enclave.so [$(Build_Mode)|$(SGX_ARCH)] success!"
	@echo
	@echo "*********************************************************************************************************************************************************"
	@echo "PLEASE NOTE: In this mode, please sign the Server_Enclave.so first using Two Step Sign mechanism before you run the app to launch and access the enclave."
	@echo "*********************************************************************************************************************************************************"
	@echo
else
all: Enclave.signed.so
endif

### Edger8r related sourcs ###
trusted/Enclave_t.c: $(SGX_EDGER8R) ./trusted/Enclave.edl
	@echo Entering ./trusted
	cd ./trusted && $(SGX_EDGER8R) --trusted ../trusted/Enclave.edl --search-path ../trusted --search-path $(SGX_SDK)/include --search-path $(PROJECT_ROOT_DIR)
	@echo "GEN  =>  $@"

trusted/Enclave_t.o: ./trusted/Enclave_t.c
	$(CC) $(Enclave_C_Flags) -c $< -o $@
	@echo "CC   <=  $<"
### Edger8r related sourcs ###

## build files needed from other directory
%_t.o: %.cpp
	$(CXX) $(Enclave_Cpp_Flags) -c $< -o $@
	@echo "CXX   <=  $<"

## build files needed from other directory
trusted/Enclave.o: trusted/Enclave.cpp
	$(CXX) $(Enclave_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

trusted/Enclave_ocall.o: trusted/Enclave_ocall.cpp
	$(CXX) $(Enclave_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

### Enclave Image ###
SRC_DIRS = ./common/ ./trusted/system/ ./trusted/concurrency_control/
CPPS = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)*.cpp))
OBJS = $(CPPS:.cpp=_t.o)


Enclave_Cpp_Objects := $(OBJS) trusted/Enclave.o trusted/Enclave_ocall.o
Enclave.so: trusted/Enclave_t.o $(Enclave_Cpp_Objects)
	$(CXX) $^ -o $@ $(Enclave_Link_Flags)
	@echo "LINK =>  $@"

### Signing ###
Enclave.signed.so: Enclave.so
	$(SGX_ENCLAVE_SIGNER) sign -key trusted/Enclave_private.pem -enclave Enclave.so -out $@ -config trusted/Enclave.config.xml
	@echo "SIGN =>  $@"
### Sources ###

### Clean command ###
clean:
	rm -f Enclave.* trusted/Enclave_t.*  $(Enclave_Cpp_Objects)
