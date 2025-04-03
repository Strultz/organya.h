PLAY := play
EXPORT := export

ifeq ($(OS), Windows_NT)
    PLAY := $(PLAY).exe
    EXPORT := $(EXPORT).exe
endif

CFLAGS := -std=c89 -pedantic -O2 -I.
LDFLAGS := -lm

.PHONY: all
all: play export

play: examples/play/play.c organya.h
	$(CC) $(CFLAGS) $(filter %.c, $^) -o $@ $(LDFLAGS)

export: examples/export/export.c organya.h
	$(CC) $(CFLAGS) $(filter %.c, $^) -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	$(RM) $(PLAY)
	$(RM) $(EXPORT)
