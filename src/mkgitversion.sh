#!/bin/sh
# Generate a C header defining GIT_HASH from the current git commit.
#
# Usage: mkgitversion.sh <output-header>
#
# The header contains a single line:
#
#     #define GIT_HASH "<short-hash>"
#
# The file is only rewritten when its content changes. When git information
# is unavailable GIT_HASH is defined as "" and is omitted by the server.
#
# This file is part of Fuzzball MUCK.  Please see LICENSE.md for details.

set -u

out=${1:?usage: mkgitversion.sh <output-header>}
hash=$(git -C "$(dirname "$out")" rev-parse --short HEAD 2>/dev/null || true)

case $hash in
    ''|*[!0-9a-f]*) hash='' ;;
esac

tmp="$out.tmp"
printf '#define GIT_HASH "%s"\n' "$hash" > "$tmp"

if cmp -s "$tmp" "$out" 2>/dev/null; then
    rm -f "$tmp"
else
    mv "$tmp" "$out"
    if [ -n "$hash" ]; then
        echo "mkgitversion.sh: GIT_HASH=$hash" >&2
    else
        echo "mkgitversion.sh: git hash unavailable, GIT_HASH empty" >&2
    fi
fi
