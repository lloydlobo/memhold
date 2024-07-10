# file: Makefile

# make watch-build-run ${PROCN}=btop
# fd -e c -e h . | entr -cprs 'make -j4 ctags'
#
# debug: NOTE(Lloyd): Use -g or -ggdb3 flag in CFLAGS. Omit -s flag.
#	$ gf2 ./memhold $(pgrep emacs)
# 	$ gdb ./memhold $(pgrep emacs)

.PHONY: all clean summary test

# Debug flags
# 0 (@public) or 1 (@internal)
DFLAGS = -DMEMHOLD_SLOW=0 -DMEMHOLD_YAGNI=0 -DMEMHOLD_PROFILE=0

BINARY = memhold

# Process name to memhold
PROCN = waybar 

SRCS = memhold.c
OBJS = $(SRCS:.c=.o)


CC = clang
CFLAGS += -std=c99
CFLAGS = -Wall -Wextra -Wno-gnu-folding-constant -Wno-sign-compare \
	 -Wno-unused-parameter -Wno-unused-variable \
	 -Wno-unused-but-set-variable -Wshadow
CFLAGS += -Werror -pedantic -pedantic-errors
CFLAGS += -Wformat -Wconversion -Wfloat-equal
CFLAGS += -fstack-protector-strong -D_FORTIFY_SOURCE=2
CFLAGS += -fsanitize=address -fsanitize=undefined -fPIE
CFLAGS += -DNDEBUG -ffast-math -march=native		# NOTE: Choices: -flto Enable Link Time Optimization
CFLAGS += -fwrapv -fno-strict-aliasing			# NOTE: Help mitigate certain aggressive optimization in Clang and GCC
CFLAGS += -pthread
CFLAGS += -O2						# NOTE: @public build
# CFLAGS += -Os						# NOTE: @public (-s strip symbols)
CFLAGS += -ferror-limit=1 -gdwarf-4 -ggdb3		# NOTE: internal build

LDLIBS = -lm


# Usage: ~
#   + make memhold
#
# See: ~
#   + cs50 Makefile guide
$(BINARY): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDLIBS) $(DFLAGS)


bench:
	hyperfine -M 1 --warmup 2 -N --show-output 'make -iB $(BINARY)' | tee -a make_bench_exe.log

build-run:
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
	@pgrep $(PROCN) | xargs -I _ ./$(BINARY) _ --verbose

summary:
	@dust --ignore-directory .git
	@tokei
	@git log --oneline

test:
	@echo 'unimplemented' # clang test.c

# Usage: ~
#   + make watch-build-run PROCN=btop
#   + make -B watch-build-run PROCN=rsibreak THRESHMEM=100M
#
watch-build-run:
	./scripts/watch_build_run.sh
