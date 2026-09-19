# Contributing

Small, focused changes are welcome. Describe the behavior and add a regression
case for changes to SQL parsing, UDF semantics, or the HTTP contract.

Install the development tools in a virtual environment, then run the same checks
as CI:

```sh
python3 -m venv .local/venv
. .local/venv/bin/activate
python3 -m pip install -r requirements-dev.txt
./scripts/lint.sh
python3 scripts/test.py
MYSQL_TEST_VERSION=8.0.46 python3 scripts/test.py
```

Tests build the native plugins and run an isolated MySQL 8.4.8 server against a
local mock API. They require Docker Compose; no API key or existing database is
used. Run the live [Sakila demo](docs/sample-data.md) separately.

Pull requests and pushes to `main` run lint and parallel MySQL 8.0.46/8.4.8 tests
on Linux AMD64. Builds also run the C++ unit tests. Required checks must pass
before merging. CI uses disposable GitHub-hosted runners, a mock API, and no
TypeSafe credentials. New pushes cancel outdated CI runs.

The separate release workflow builds and tests the extracted bundles on Linux
AMD64 and ARM64. A `v*` tag creates a draft release with checksums; review it
before publishing. Running that workflow manually tests bundles and uploads
workflow artifacts without creating a release. Only the release publishing job
has repository write permission. Actions are pinned to commit hashes; update
those pins when upgrading an action.

Never commit credentials or sample database dumps. New SQL syntax must preserve
MySQL precedence and quoting, or reject the query explicitly. API failures must
raise SQL errors rather than silently exclude rows.

To verify a downloaded or extracted release bundle against stock MySQL:

```sh
python3 scripts/test.py --bundle dist
MYSQL_TEST_VERSION=8.0.46 python3 scripts/test.py --bundle dist
```

Code is licensed under GPL-2.0-only; contributions use the same license.
