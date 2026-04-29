all:
	$(MAKE) -C out/build

%:
	$(MAKE) -C out/build $@
