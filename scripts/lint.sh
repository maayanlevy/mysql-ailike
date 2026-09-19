#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
python3 -m ruff check --isolated --select E4,E7,E9,F scripts tests
for script in scripts/*.sh; do
  sh -n "$script"
done
