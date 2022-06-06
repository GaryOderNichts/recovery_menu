#-------------------------------------------------------------------------------
.SUFFIXES:
#-------------------------------------------------------------------------------

.PHONY: all clean ios_kernel ios_mcp

all: recovery_menu
	@echo "\033[92mDone!\033[0m"

recovery_menu: ios_kernel
	python3 build_recovery_file.py $@

ios_mcp:
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios_mcp

ios_kernel: ios_mcp
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios_kernel

clean:
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios_kernel clean
	@$(MAKE) --no-print-directory -C $(CURDIR)/ios_mcp clean
	rm -f recovery_menu
