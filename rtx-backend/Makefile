# Makefile for rtx-backend
# Cross-kompilering for MIPS32 BE med msdk-toolchain
#
# Bruk:
#   make          → native bygg (for testing på host)
#   make CROSS=1  → MIPS cross-kompilering

TOOLCHAIN ?= /toolchain/msdk-4.8.5-mips-EB-4.4-u0.9.33-m32ut-180206
CROSS_PREFIX := $(TOOLCHAIN)/bin/msdk-linux-

ifdef CROSS
CC  := $(CROSS_PREFIX)gcc
AR  := $(CROSS_PREFIX)ar
else
CC  := gcc
AR  := ar
endif

CFLAGS  := -Wall -Os -I src
LDFLAGS :=

SRC := src/main.c src/metrics.c src/hnap.c src/mongoose.c
OBJ := $(SRC:.c=.o)
BIN := rtx-backend

.PHONY: all clean

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJ) $(BIN)

# Last ned mongoose hvis den mangler
src/mongoose.c src/mongoose.h:
	@echo "Laster ned mongoose..."
	curl -sL https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.c -o src/mongoose.c
	curl -sL https://raw.githubusercontent.com/cesanta/mongoose/master/mongoose.h -o src/mongoose.h
