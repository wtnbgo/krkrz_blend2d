ifeq ($(shell type cygpath > /dev/null && echo true),true)
FIXPATH = cygpath -ma
else
FIXPATH = realpath
endif

VCPKG=$(shell $(FIXPATH) "$(VCPKG_ROOT)/vcpkg")

ifeq ($(VCPKG_TARGET_TRIPLET),)
VCPKG_TARGET_TRIPLET=x86-windows-static
endif

ifeq ($(BUILD_TYPE),)
BUILD_TYPE=Debug
endif

export VCPKG_TARGET_TRIPLET
export BUILD_TYPE

ifeq ($(DATAPATH),)
DATAPATH=data
endif

DATAPATH_ABS=$(shell $(FIXPATH) "$(DATAPATH)")

BUILD_PATH=build/$(VCPKG_TARGET_TRIPLET)/$(BUILD_TYPE)

.PHONY: build  prebuild

# ビルド実行
# msys 環境ではMSVCコンパイラとリンカの探索に失敗するが
# CC=cl make prebuild で回避可能
# CMAKEOPT で引数定義追加
prebuild:
	cmake -G Ninja -DVCPKG_TARGET_TRIPLET=$(VCPKG_TARGET_TRIPLET) -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) ${CMAKEOPT} -B $(BUILD_PATH)

build:
	cmake --build $(BUILD_PATH)

clean:
	cmake --build $(BUILD_PATH) --target clean

