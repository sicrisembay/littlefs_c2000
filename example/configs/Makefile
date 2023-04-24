PHONY := create_folder

KCONFIG_OUTPUT_LOC := $(CURDIR)/generated

menuconfig: create_folder
	menuconfig $(ROOT_KCONFIG)
	cd $(KCONFIG_OUTPUT_LOC) && genconfig $(ROOT_KCONFIG) && mv config.h autoconf.h

loadconfig: create_folder
	defconfig --kconfig $(ROOT_KCONFIG) $(DEFCONFIG_FILE)
	cd $(KCONFIG_OUTPUT_LOC) && genconfig $(ROOT_KCONFIG) && mv config.h autoconf.h

saveconfig: create_folder
	cp .config $(DEFCONFIG_FILE)

create_folder:
	mkdir -p $(KCONFIG_OUTPUT_LOC)