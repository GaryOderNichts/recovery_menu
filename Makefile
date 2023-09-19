#-------------------------------------------------------------------------------
.SUFFIXES:
#-------------------------------------------------------------------------------

.PHONY: all clean ios_kernel ios_mcp ios_fs

all: recovery_menu
	@echo "\033[92mDone!\033[0m"

recovery_menu: ios_kernel
	python3 build_recovery_file.py $@

ios_mcp:
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios_mcp

ios_fs:
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios_fs

ios_kernel: ios_mcp ios_fs
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios_kernel

clean:
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios_kernel clean
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios_mcp clean
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios_fs clean
	rm -f recovery_menu
