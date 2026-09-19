# Contributing

Small, focused changes are welcome. Describe the behavior and add a regression
case for changes to SQL parsing, UDF semantics, or the HTTP contract.

```sh
python3 scripts/test.py
MYSQL_TEST_VERSION=8.0.46 python3 scripts/test.py
```

Tests build the native plugins and run an isolated MySQL 8.4.8 server against a
local mock API. They require Docker Compose; no API key or existing database is
used. Run the live [Sakila demo](docs/sample-data.md) separately.

Never commit credentials or sample database dumps. New SQL syntax must preserve
MySQL precedence and quoting, or reject the query explicitly. API failures must
raise SQL errors rather than silently exclude rows.

To verify a downloaded or extracted release bundle against stock MySQL:

```sh
python3 scripts/test.py --bundle dist
MYSQL_TEST_VERSION=8.0.46 python3 scripts/test.py --bundle dist
```

Code is licensed under GPL-2.0-only; contributions use the same license.
