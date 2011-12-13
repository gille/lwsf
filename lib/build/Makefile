SILENT=@
AR=$(SILENT)ar
CC=$(SILENT)gcc
CFLAGS=-c -O3 -g
ASFLAGS=-c -Wa,-g
LDFLAGS=-g

ARCH=x86_32

OBJS  = lwsf.o 
OBJS += sockets.o
OBJS += arch.o 
OBJS += arch_asm.o

ARFLAGS=-rc
CFLAGS += -I../src/include
CFLAGS += -I../include

vpath %.c ../arch/$(ARCH)
vpath %.S ../arch/$(ARCH)
vpath %.c ../src

all: liblwsf.a

liblwsf.a: $(OBJS)
	$(AR) $(ARFLAGS) $@ $(OBJS)
%.o: %.c
	$(CC) $(CFLAGS) -o $@ $<
%.o: %.S
	$(CC) $(ASFLAGS) -o $@ $<

clean: 
	$(RM) -f *.o *.a
