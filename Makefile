# file: Makefile

# TOP

.PHONY: all clean summary test

# debug: NOTE(Lloyd): Use -g or -ggdb3 flag in CFLAGS. Omit -s flag.
#	$ gf2 ./memhold $(pgrep emacs)
# 	$ gdb ./memhold $(pgrep emacs)



BINARY = memhold

# Process name to memhold
#
PROCN = waybar 

SRCS = memhold.c
OBJS = $(SRCS:.c=.o)



CC = clang
CFLAGS += -std=c99 -pthread
CFLAGS = -Wall -Wextra -Wno-gnu-folding-constant -Wno-sign-compare \
	 -Wno-unused-parameter -Wno-unused-variable \
	 -Wno-unused-but-set-variable -Wshadow
CFLAGS += -Werror
CFLAGS += -DNDEBUG -ffast-math -march=native
# CFLAGS += -Os ###> @public (-s strip symbols)
# CFLAGS += -O3 ###> @public?
CFLAGS += -ferror-limit=1 -gdwarf-4 -ggdb3 -O0 ###> @internal @debug

LDLIBS = -lm

# 0 (@public) or 1 (@internal)
DFLAGS = -DMEMHOLD_SLOW=0 -DMEMHOLD_YAGNI=0




# Usage: ~
#   + make memhold
#
# See: ~
#   + cs50 Makefile guide
$(BINARY): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS) $(DFLAGS)




bench:
	hyperfine -M 1 --warmup 2 -N --show-output 'make -iB $(BINARY)' | tee -a make_bench_exe.log

build_run:
	make -B $(BINARY) && make run

clean:
	@echo 'unimplemented' # trash ./build

ctags:
	./scripts/watch_ctags.sh

format:
	clang-format -i --verbose *.{c,h}

# Usage: ~
#   + make run PROCN=tmux
#
run:
	@pgrep $(PROCN) | xargs -I _ ./$(BINARY) _

summary:
	@dust --ignore-directory .git
	@tokei
	@git log --oneline

test:
	@echo 'unimplemented' # clang test.c

# Usage: ~
#   + make watch_build_run PROCN=btop
#   + make -B watch_build_run PROCN=rsibreak THRESHMEM=100M
#
watch_build_run:
	./scripts/watch_build_run.sh


# BOT
