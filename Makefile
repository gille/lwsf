lib:
	$(MAKE) -C lib/build/ all
apps:
	$(MAKE) -C apps/build/ all

all: 
	$(MAKE) -C lib/build/ all
	$(MAKE) -C apps/build/ all
clean:
	$(MAKE) -C lib/build/ clean
	$(MAKE) -C apps/build/ clean


