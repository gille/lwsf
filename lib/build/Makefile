SILENT=@
AR=$(SILENT)ar
CC=$(SILENT)gcc
RM=$(SILENT)rm
CFLAGS  = -c
CFLAGS += -O3
CFLAGS += -g
CFLAGS += -Wall 
CFLAGS += -fconserve-stack

ASFLAGS=-c -Wa,-g
LDFLAGS=-g

ARCH=x86_32

OBJS  = lwsf.o 
OBJS += sockets.o
OBJS += mem_cache.o
OBJS += arch.o 
OBJS += arch_asm.o

ARFLAGS=-rc
CFLAGS += -I../src/include
CFLAGS += -I../include
CFLAGS += -I../arch/$(ARCH)/

vpath %.c ../arch/$(ARCH)
vpath %.S ../arch/$(ARCH)
vpath %.c ../src

all: liblwsf.a

liblwsf.a: $(OBJS) ../src/include/list.h
	$(AR) $(ARFLAGS) $@ $(OBJS)
%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<
%.o: %.S
	$(CC) $(ASFLAGS) -o $@ $<

clean: 
	$(RM) -f *.o *.a
