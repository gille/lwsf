SILENT=@
AR=$(SILENT)ar
CC=$(SILENT)gcc
RM=$(SILENT)rm
CFLAGS=-O3 -g -Wall
ASFLAGS=-Wa,-g
LDFLAGS=-g

CFLAGS += -I../src/include
CFLAGS += -I../include
CFLAGS += -I../../lib/include

LIBS+=-L../../lib/build -llwsf

vpath %.c ../arch/$(ARCH)
vpath %.S ../arch/$(ARCH)
vpath %.c ../src

APPS = test1 test2 test3 test4 hackysack

all: $(APPS)

%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<
%.o: %.S
	$(CC) $(ASFLAGS) -o $@ $<

clean: 
	$(RM) -f *.o *.a $(APPS)