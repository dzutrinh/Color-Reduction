# Make file for static linked applications
ifeq ($(OS), Windows_NT)
	SEP=\\
else
	SEP=/
endif

UNIPAL=unipal
OCTREE=octree
COLORMATCH=colormatch

ifeq ($(OS), Windows_NT)
RM=del
CFLAGS=-s
else
	RM=rm -f
	CFLAGS=
endif
CC=gcc
CFLAGS+=-Wall -O2 -std=c99
SRC=unipal.c image.c bitmap.c dithering.c kdtree.c
OCTREE_SRC=octree.c image.c bitmap.c dithering.c kdtree.c
COLORMATCH_SRC=colormatch.c image.c bitmap.c dithering.c kdtree.c

all: $(UNIPAL) $(OCTREE) $(COLORMATCH)

$(UNIPAL): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@

$(OCTREE): $(OCTREE_SRC)
	$(CC) $(CFLAGS) $(OCTREE_SRC) -o $@

$(COLORMATCH): $(COLORMATCH_SRC)
	$(CC) $(CFLAGS) $(COLORMATCH_SRC) -o $@

clean:
ifeq ($(OS), Windows_NT)
	$(RM) $(UNIPAL).exe $(OCTREE).exe $(COLORMATCH).exe
else
	$(RM) $(UNIPAL) $(OCTREE) $(COLORMATCH)
endif
