#!/usr/bin/env bash

# file: watch_ctags.sh

# TOP

# Usage:
# 	```shell
# 	$ fd --extension c --extension h . | entr -r make ctags
# 	```

set -xe

file tags
cp -r tags tags.bak
fd --extension c --extension h . | xargs ctags -R
wc --lines tags && file tags
sha256sum tags tags.bak

# BOT
