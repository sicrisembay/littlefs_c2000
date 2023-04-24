PHONY := menuconfig loadconfig saveconfig

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