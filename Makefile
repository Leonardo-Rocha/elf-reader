# Makefile for the project.
# Best viewed with tabs set to 4 spaces.

CC = gcc
LD = ld
BI = ./bin
BU = ./build
RC = ./src

# Where to locate the kernel in memory
KERNEL_ADDR	= 0x1000

# Compiler flags
#-fno-builtin:		Don't recognize builtin functions that do not begin
#			with '__builtin_' as prefix.
#
#-fomit-frame-pointer:	Don't keep the frame pointer in a register for 
#			functions that don't need one.
#
#-make-program-do-what-i-want-it-to-do:
#			Turn on all friendly compiler flags.
#
#-O2:			Turn on all optional optimizations except for loop
#			unrolling and function inlining.
#
#-c:			Compile or assemble the source files, but do not link.
#
#-Wall:			All of the `-W' options combined (all warnings on)

CCOPTS = -Wall -g -m32 -c -fomit-frame-pointer -O2 -fno-builtin

# Linker flags
#-nostartfiles:	Do not use the standard system startup files when linking.
#
#-nostdlib:	Don't use the standard system libraries and startup files
#		when linking. Only the files you specify will be passed
#		to the linker.
#          

LDOPTS = -nostartfiles -nostdlib -melf_i386

# Makefile targets

all: bootblock buildimage kernel image

kernel: $(BI)/kernel.o
	$(LD) $(LDOPTS) -Ttext $(KERNEL_ADDR) -o $(BU)/kernel $<

bootblock: $(BI)/bootblock.o
	$(LD) $(LDOPTS) -Ttext 0x0 -o $(BU)/bootblock $<

buildimage: $(BI)/buildimage.o
	$(CC) -o $(BU)/buildimage $<

# Build an image to put on the floppy
image: $(BU)/bootblock $(BU)/buildimage $(BU)/kernel
	$(BU)/buildimage --extended $(BU)/bootblock $(BU)/kernel


# Put the image on the usb stick (these two stages are independent, as both
# vmware and bochs can run using only the image file stored on the harddisk)	
boot: image
	dd if=./image of=/dev/sdb bs=512

# Clean up!
# Cannot delete bootblock.o
clean:
	rm -f $(BU)/*
	rm -f $(BI)/buildimage.o $(BI)/kernel.o

# No, really, clean up!
distclean: clean
	rm -f *~
	rm -f \#*
	rm -f *.bak
	rm -f serial.out
	rm -f bochsout.txt

# How to compile buildimage
$(BI)/buildimage.o:
	$(CC) -c -o $@ $(RC)/buildimage.c

# How to compile a C file
$(BI)/%.o:$(RC)/%.c
	$(CC) $(CCOPTS) -o $@ $<

# How to assemble
$(BI)/%.o:$(RC)/%.s
	$(CC) $(CCOPTS) -o $@ $<

# How to produce assembler input from a C file
$(RC)/%.s:$(RC)/%.c
	$(CC) $(CCOPTS) -S -o $@ $<
