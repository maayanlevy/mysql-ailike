#!/bin/sh
# Build installable native modules without starting or connecting to MySQL.
set -eu
cd "$(dirname "$0")/.."
if [ -n "${PLATFORM:-}" ]; then
  docker build --platform "$PLATFORM" --target plugin --output "type=local,dest=dist" .
else
  docker build --target plugin --output "type=local,dest=dist" .
fi
