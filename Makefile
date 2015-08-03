CC=arm-none-eabi-gcc
CFLAGS=-fPIE -fno-zero-initialized-in-bss -std=c99 -mcpu=mpcore -fshort-wchar -O2 -Wall -shared -static
ASFLAGS=-nostartfiles -nostdlib
LD=arm-none-eabi-gcc
LDFLAGS=-T uvl.x -nodefaultlibs -nostdlib
OBJCOPY=arm-none-eabi-objcopy
OBJCOPYFLAGS=
DATSIZE=0x300
BINDIR=bin/
CODDIR=code/
ROPDIR=rop/
BROWSERIFY=$(BINDIR)browserify

FILES=downfiles.bin

TARGETS=$(addprefix $(BINDIR), $(FILES))

all: $(TARGETS)

%.o: $(CODDIR)%.c
	$(CC) -c -o $@ $< $(CFLAGS)

%.ro: $(ROPDIR)%.S
	$(CC) -c -o $@ $< $(ASFLAGS)

%.elf: %.o
	$(LD) -o $@ $^ $(LDFLAGS)

$(BINDIR)%.bin: %.elf
	$(OBJCOPY) -O binary $^ $@

$(BINDIR)%.dat: %.ro
	$(OBJCOPY) --pad-to $(DATSIZE) -O binary $^ $@

$(BINDIR)%.js: $(BINDIR)%.dat
	$(BROWSERIFY) -c $^ > $@

$(BINDIR)%.js: $(BINDIR)%.bin
	$(BROWSERIFY) $^ > $@

.PHONY: clean bindir browserify

clean:
	rm -rf *~ *.o *.elf *.bin *.s *.dat

$(TARGETS): | bindir browserify

bindir:
	mkdir -p $(BINDIR)

browserify:
	gcc -o $(BROWSERIFY) $(CODDIR)browserify.c