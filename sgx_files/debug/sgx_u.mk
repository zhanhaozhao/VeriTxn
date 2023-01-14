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
	Urts_Library_Name := sgx_urts_sim
else
	Urts_Library_Name := sgx_urts
endif

ifeq ($(SGX_MODE), HW)
ifneq ($(SGX_DEBUG), 1)
ifneq ($(SGX_PRERELEASE), 1)
Build_Mode = HW_RELEASE
endif
endif
endif
### Intel(R) SGX SDK Settings ###

### Project Settings ###
Common_C_Cpp_Flags := $(SGX_COMMON_CFLAGS) -fPIC -Wno-attributes -I.
Common_C_Cpp_Flags += -Wall -Wextra -Winit-self -Wpointer-arith -Wreturn-type \
                    -Waddress -Wsequence-point -Wformat-security \
                    -Wmissing-include-dirs -Wfloat-equal -Wundef -Wshadow \
                    -Wcast-align -Wcast-qual -Wconversion -Wredundant-decls -D_GLIBCXX_USE_CXX11_ABI=0
Common_C_Flags := -Wjump-misses-init -Wstrict-prototypes \
										-Wunsuffixed-float-constants
SGX_RA_TLS_Extra_Flags := -DWOLFSSL_SGX 


# Three configuration modes - Debug, prerelease, release
#   Debug - Macro DEBUG enabled.
#   Prerelease - Macro NDEBUG and EDEBUG enabled.
#   Release - Macro NDEBUG enabled.
ifeq ($(SGX_DEBUG), 1)
        Common_C_Cpp_Flags += -DDEBUG -UNDEBUG -UEDEBUG
else ifeq ($(SGX_PRERELEASE), 1)
        Common_C_Cpp_Flags += -DNDEBUG -DEDEBUG -UDEBUG
else
        Common_C_Cpp_Flags += -DNDEBUG -UEDEBUG -UDEBUG
endif

App_C_Cpp_Flags := $(Common_C_Cpp_Flags) $(SGX_RA_TLS_Extra_Flags) -Iuntrusted -I$(SGX_SDK)/include -I$(PROJECT_ROOT_DIR) 
App_C_Cpp_Flags += -I$(PROJECT_ROOT_DIR)/common -I$(PROJECT_ROOT_DIR)/untrusted -I$(PROJECT_ROOT_DIR)/untrusted/benchmarks -I$(PROJECT_ROOT_DIR)/untrusted/cache -Iuntrusted/system
### Project Settings ###

### Linking setting ###
App_Link_Flags := -L$(SGX_LIBRARY_PATH)	-l$(Urts_Library_Name) \
	-lpthread -lz -lm -lcrypto -lrt -lnanomsg  -lrocksdb

## Add sgx_uae_service library to link ##
ifneq ($(SGX_MODE), HW)
	App_Link_Flags += -lsgx_uae_service_sim
else
	App_Link_Flags += -lsgx_uae_service
endif
### Linking setting ###

### Phony targets ###
.PHONY: all clean

### Build all ###
ifeq ($(Build_Mode), HW_RELEASE)
all: App
	@echo "Build App [$(Build_Mode)|$(SGX_ARCH)] success!"
	@echo
	@echo "*********************************************************************************************************************************************************"
	@echo "PLEASE NOTE: In this mode, please sign the Worker_Enclave.so first using Two Step Sign mechanism before you run the app to launch and access the enclave."
	@echo "*********************************************************************************************************************************************************"
	@echo

else
all: App
endif

### Sources ###
## Edger8r related sources ##
untrusted/Enclave_u.c: $(SGX_EDGER8R) trusted/Enclave.edl
	@echo Entering ./untrusted
	cd ./untrusted && $(SGX_EDGER8R) --untrusted ../trusted/Enclave.edl --search-path ../trusted --search-path $(SGX_SDK)/include --search-path $(PROJECT_ROOT_DIR)
	@echo "GEN  =>  $@"

untrusted/Enclave_u.o: untrusted/Enclave_u.c
	$(CC) $(Common_C_Flags) $(App_C_Cpp_Flags) -c $< -o $@
	@echo "CC   <=  $<"
## Edger8r related sources ##

## build files needed from other directory

%_u.o: %.cpp
	$(CXX) $(App_C_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

untrusted/Enclave_ecall.o: untrusted/Enclave_ecall.cpp
	$(CXX) $(App_C_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

untrusted/ocall_patches.o: untrusted/ocall_patches.cpp
	$(CXX) $(App_C_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

App.o: main.cpp
	$(CXX) $(App_C_Cpp_Flags) -c $< -o $@
	@echo "CXX  <=  $<"

SRC_DIRS = ./common/ ./untrusted/system/ ./untrusted/cache/ ./untrusted/benchmarks/
CPPS = $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)*.cpp))
OBJS = $(CPPS:.cpp=_u.o)

# libuntrusted.a: $(OBJS) untrusted/Enclave_u.o untrusted/Enclave_ecall.o
# 	ar -rcs $@ $^
# 	@echo "LINK => $@"

## Build worker app ##
# App: App.o libuntrusted.a 
App: App.o $(OBJS) untrusted/Enclave_u.o untrusted/Enclave_ecall.o untrusted/ocall_patches.o
	$(CXX) $^ -o $@ -L. $(App_Link_Flags)
	@echo "LINK =>  $@"
### Sources ###

### Clean command ###
clean:
	rm -f App *.o untrusted/*.o $(OBJS) untrusted/Enclave_u.* libuntrusted.a