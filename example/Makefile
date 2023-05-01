PHONY := menuconfig loadconfig saveconfig protobuf

ROOT_KCONFIG := $(CURDIR)/Kconfig
FILESYSTEM_LOC := $(CURDIR)/../filesystem
DEFCONFIG_FILE := lfs_defconfig
KCONFIG_CONFIG := $(CURDIR)/configs/.config

export ROOT_KCONFIG DEFCONFIG_FILE KCONFIG_CONFIG

menuconfig:
	cd configs && make menuconfig

loadconfig:
	cd configs && make loadconfig

saveconfig:
	cd configs && make saveconfig

PB_GENERATOR := $(abspath $(CURDIR)/../third_party/nanopb/generator)/nanopb_generator.py
PB_PROTO_LOC := $(abspath $(FILESYSTEM_LOC)/protobuf)

protobuf:
	cd $(PB_PROTO_LOC) && python $(PB_GENERATOR) test.proto
