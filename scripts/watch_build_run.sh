#!/usr/bin/env bash

# file: watch_build_run.sh

# TOP

# Usage:
# 	```shell
# 	$ make watch_build_run
# 	```

fd --extension c --extension h . | entr -cpr xargs make -j4 build-run

# BOT
