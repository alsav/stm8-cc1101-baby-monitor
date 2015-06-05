CC=sdcc
CFLAGS=-mstm8 --opt-code-size
CFLAGS+=-I/usr/local/share/sdcc/include
CFLAGS+=-I.
CFLAGS+=-I$(STM8LIBINC)

STM8LIBDIR=../stm8_lib
SDCCLIBSMALL=/usr/local/share/sdcc/lib/small

#STM8LIBINC=$(STM8LIBDIR)/inc
STM8LIBINC=./inc
DEFINES=STM8S003
SRC=main
OUT_DIR=build

#SRCS= ministdlib.c hw.c ctrl.c pid.c uart.c
SRCS= ministdlib.c uart.c hw.c

OBJS = $(SRCS:%.c=$(OUT_DIR)/%.rel)

STDLIBNAME=libstm8s003.a

TARG=$(OUT_DIR)/$(SRC).hex

all: precomp $(TARG) postcomp

precomp:
	mkdir -p $(OUT_DIR)

postcomp:
	packihx $(OUT_DIR)/$(SRC).ihx > $(SRC).hex


$(TARG): $(OBJS)
	$(CC) $(CFLAGS) $(LFLAGS) -D $(DEFINES) -L $(SDCCLIBSMALL) -o $(OUT_DIR)/$(SRC).ihx $(SRC).c $(OBJS)


$(OUT_DIR)/%.rel: %.c
	$(CC) -c $(CFLAGS) $(LFLAGS) -D $(DEFINES) -L $(SDCCLIBSMALL) -c -o $@ $<




pwron:
	vsprog -V"tvcc.set 3300"

pwroff:
	vsprog -V"tvcc.set 0"


prepare:
	packihx $(OUT_DIR)/$(SRC).ihx > $(OUT_DIR)/$(SRC).hex

clean:
	rm -Rrf $(OUT_DIR)
	rm -f $(SRC).hex

erase: pwroff
	vsprog -cstm8s103f3 -ms -oe -V"tvcc.set 3300"
	vsprog -V"tvcc.set 0" -V"tvcc.set 3300"



flash: erase
	vsprog -cstm8s103f3 -ms -owf -I $(SRC).hex -V"tvcc.set 3300"
	vsprog -V"tvcc.set 0" -V"tvcc.set 3300"
#	vsprog -V"tvcc.set 0" -V"tvcc.set 3300" -V"tvcc.set 3300"
