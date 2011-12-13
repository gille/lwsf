SILENT=@
AR=$(SILENT)ar
CC=$(SILENT)gcc
CFLAGS=-O3 -g
ASFLAGS=-Wa,-g
LDFLAGS=-g

ARCH=x86_32

ARFLAGS=-rc
CFLAGS += -I../src/include
CFLAGS += -I../include
CFLAGS += -I../../lib/include

LIBS+=-L../../lib/build -llwsf

vpath %.c ../arch/$(ARCH)
vpath %.S ../arch/$(ARCH)
vpath %.c ../src

APPS = test1

all: test1

test1: test1.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) $(LIBS)

%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<
%.o: %.S
	$(CC) $(ASFLAGS) -o $@ $<

clean: 
	$(RM) -f *.o *.a $(APPS)