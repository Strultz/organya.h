ifneq ($(OS),Windows_NT)
    LDFLAGS += -lm -lpthread
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        LDFLAGS += -ldl
    endif
endif

play: play.c
	gcc -std=c89 -Wall -Wextra -Wpedantic -Werror -Wno-maybe-uninitialized -Wno-unused-result -O2 -I../../ play.c -o $@ $(LDFLAGS)
# gcc complains about miniaudio with -Wmaybe-uninitialized and -Wunused-result