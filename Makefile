ifeq ($(VCPKG_ROOT),)
$(error Variables VCPKG_ROOT not set correctly.)
endif

ifeq ($(shell type cygpath > /dev/null && echo true),true)
FIXPATH = cygpath -ma
else
FIXPATH = realpath
endif

VCPKG=$(shell $(FIXPATH) "$(VCPKG_ROOT)/vcpkg")

ifeq ($(VCPKG_TARGET_TRIPLET),)
ifeq ($(OS),Windows_NT)
	# USE MSVC
	export CC=cl.exe
	VCPKG_TARGET_TRIPLET=$(VSCMD_ARG_TGT_ARCH)-windows-static
else
    UNAME_S := $(shell uname -s)
    UNAME_P := $(shell uname -p)
	VCPKG_TARGET_TRIPLET=$(UNAME_P)-$(UNAME_S)
endif
endif

ifeq ($(TARGET_TRIPLET),)
ifeq ($(OS),Windows_NT)
	TARGET_TRIPLET=$(VSCMD_ARG_TGT_ARCH)-windows
else
    UNAME_S := $(shell uname -s)
    UNAME_P := $(shell uname -p)
	TARGET_TRIPLET=$(UNAME_P)-$(UNAME_S)
endif
endif

ifeq ($(BUILD_TYPE),)
BUILD_TYPE=Release
endif

export VCPKG_TARGET_TRIPLET
export BUILD_TYPE

BUILD_PATH=build/$(VCPKG_TARGET_TRIPLET)/$(BUILD_TYPE)

all: build

.PHONY: build prebuild run clean

# ビルド実行
# msys 環境ではMSVCコンパイラとリンカの探索に失敗するが
# CC=cl make prebuild で回避可能
# CMAKEOPT で引数定義追加
prebuild:
	cmake -G Ninja -DVCPKG_TARGET_TRIPLET=$(VCPKG_TARGET_TRIPLET) -DTARGET_TRIPLET=$(TARGET_TRIPLET) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) ${CMAKEOPT} -B $(BUILD_PATH)

build:
	cmake --build $(BUILD_PATH)

runproj:
	cmake --build $(BUILD_PATH) --target run

clean:
	cmake --build $(BUILD_PATH) --target clean

run:
	./krkrz_new.exe -contfreq=60 -- data 
