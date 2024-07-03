# file: Makefile

# TOP

.PHONY: all clean test

CC = clang
CFLAGS = -Wall -Wextra -Wno-gnu-folding-constant -Wno-sign-compare \
	 -Wno-unused-parameter -Wno-unused-variable \
	 -Wno-unused-but-set-variable -Wshadow -Werror
CFLAGS += -DNDEBUG -ffast-math -march=native
CFLAGS += -std=c99 -O3
# CFLAGS += -std=c99 -ferror-limit=1 -gdwarf-4 -ggdb3 -O0
# CFLAGS += -s

# =0 or =1
DFLAGS = -DMEMHOLD_SLOW=0

LDLIBS = -lm -lpthread

EXE = memhold

SRCS = main.c
OBJS = $(SRCS:.c=.o)

# Usage: ~
#   + make memhold
#
# See: ~
#   + cs50 Makefile guide
$(EXE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS) $(DFLAGS)

bench:
	hyperfine -M 1 --warmup 2 -N --show-output 'make -iB $(EXE)' | tee -a make_bench_exe.log

build_run:
	make -B $(EXE) && make run

clean:
	@echo 'unimplemented' # trash ./build

ctags:
	./scripts/watch_ctags.sh

format:
	clang-format -i --verbose *.{c,h}

run:
	./$(EXE)

test:
	@echo 'unimplemented' # clang test.c

watch_build_run:
	./scripts/watch_build_run.sh


# BOT
