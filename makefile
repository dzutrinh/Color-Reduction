# Make file for static linked applications
ifeq ($(OS), Windows_NT)
	SEP=\\
else
	SEP=/
endif

UNIPAL=unipal

ifeq ($(OS), Windows_NT)
RM=del
CFLAGS=-s
else
	RM=rm -f
	CFLAGS=
endif
CC=gcc
CFLAGS+=-Wall -O2 -std=c99
SRC=unipal.c image.c bitmap.c

all: $(UNIPAL)

$(UNIPAL): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@

clean:
ifeq ($(OS), Windows_NT)
	$(RM) $(UNIPAL).exe
else
	$(RM) $(UNIPAL)
endif
