ifneq ($(OS),Windows_NT)
    LDFLAGS += -lm
endif

export: export.c
	gcc -std=c89 -Wall -Wextra -Wpedantic -Werror -O2 -I../../ export.c -o $@ $(LDFLAGS)