# file: Makefile

# TOP

.PHONY: all clean summary test

# debug: NOTE(Lloyd): Use -g or -ggdb3 flag in CFLAGS. Omit -s flag.
#	$ gf2 ./memhold $(pgrep emacs)
# 	$ gdb ./memhold $(pgrep emacs)

CC = clang
CFLAGS = -Wall -Wextra -Wno-gnu-folding-constant -Wno-sign-compare \
	 -Wno-unused-parameter -Wno-unused-variable \
	 -Wno-unused-but-set-variable -Wshadow 
CFLAGS += -Werror
CFLAGS += -DNDEBUG -ffast-math -march=native
CFLAGS += -std=c99 -O3  -pthread
# CFLAGS += -std=c99 -ferror-limit=1 -gdwarf-4 -ggdb3 -O0
# CFLAGS += -s

# =0 or =1
DFLAGS = -DMEMHOLD_SLOW=0 -DMEMHOLD_YAGNI=0

LDLIBS = -lm

EXE = memhold
PROCN = waybar 
# Process name to memhold

SRCS = memhold.c
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

# $ make run PROCN=tmux
run:
	@pgrep $(PROCN) | xargs -I _ ./$(EXE) _

summary:
	@dust --ignore-directory .git
	@tokei
	@git log --oneline

test:
	@echo 'unimplemented' # clang test.c

# $ make watch_build_run PROCN=btop
watch_build_run:
	./scripts/watch_build_run.sh


# BOT
