#!/usr/bin/env python3
"""Download and verify the public Sakila sample; never connect to MySQL."""

import hashlib
from pathlib import Path
import urllib.error
import urllib.request


COMMIT = "e089a5b1ec9af0df7a9c6a5d47d49fa1736a4e84"
BASE_URL = f"https://raw.githubusercontent.com/jOOQ/sakila/{COMMIT}"
DESTINATION = Path(__file__).resolve().parents[1] / ".local" / "sakila"
FILES = (
    (
        "sakila-schema.sql",
        "mysql-sakila-db/mysql-sakila-schema.sql",
        "f2c41c3bf6d6c239941b4f98fb37afad21f6be12b82bf586202529a793ccc2ee",
    ),
    (
        "sakila-data.sql",
        "mysql-sakila-db/mysql-sakila-insert-data.sql",
        "353ef858e4d2d1a60549969283434da15b905aef3ca7099d82b649b75f8de99f",
    ),
    (
        "LICENSE",
        "LICENSE",
        "1f1f3467bc7e7ba6277f20ddd7925e2ec2165d3bd56bdaf2eb26ea84c3953279",
    ),
)


def main():
    DESTINATION.mkdir(parents=True, exist_ok=True)
    for name, source, expected in FILES:
        path = DESTINATION / name
        if path.is_file() and hashlib.sha256(path.read_bytes()).hexdigest() == expected:
            print(path)
            continue
        with urllib.request.urlopen(f"{BASE_URL}/{source}", timeout=60) as response:
            content = response.read()
        if hashlib.sha256(content).hexdigest() != expected:
            raise ValueError(f"SHA-256 mismatch for {source}; file was not saved")
        path.write_bytes(content)
        print(path)


if __name__ == "__main__":
    try:
        main()
    except (OSError, urllib.error.URLError, ValueError) as error:
        raise SystemExit(str(error)) from error
