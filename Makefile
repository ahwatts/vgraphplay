all:
	$(MAKE) -C build

%:
	$(MAKE) -C build $@
