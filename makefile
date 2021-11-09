.PHONY: all
all:
	$(MAKE) -C server
	$(MAKE) -C client

.PHONY: clean
clean:
	$(MAKE) clean -C server
	$(MAKE) clean -C client