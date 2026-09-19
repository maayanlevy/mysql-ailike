---
name: mysql-ailike
description: Install, query, and validate the AILIKE MySQL plugin when adding natural-language row filters, comparing two columns with a prompted relationship, integrating application SQL, or troubleshooting syntax and configuration.
license: GPL-2.0-only
---

# AILIKE for MySQL

AILIKE evaluates a natural-language condition against one text value or a
relationship between two text values using TypeSafe Jev. Use it for semantic
judgments; keep exact identifiers, arithmetic, date comparisons, and ordinary
joins in SQL. Three-argument comparisons require v0.2.0 or newer.

## Installation and configuration

For installation, read the repository's `docs/install.md` or the
[installation guide](https://github.com/maayanlevy/mysql-ailike/blob/main/docs/install.md).
Follow its bundle download, checksum, dependency, and registration steps for the
user's target server. Verified targets are MySQL 8.0.46 and 8.4.8 on Linux AMD64
and ARM64; do not infer compatibility with other versions or MariaDB.

- `ailike_udf.so` provides `ailike(value, prompt)` and
  `ailike(left, right, prompt)`. `ailike_rewrite.so` enables infix syntax by
  rewriting it before MySQL parses the statement. It is not a SQL function for
  callers to invoke. Function-only installation supports both function forms.
- When upgrading an existing installation, follow the guide's upgrade procedure:
  stop AILIKE queries and unregister it before replacing the libraries, then
  register it again. Do not overwrite a loaded native library.
- Configure `TYPESAFE_API_KEY` or `TYPESAFE_API_KEY_FILE` in the **mysqld process
  environment**. A key file takes precedence and must be readable by MySQL.
  Setting a variable only in the SQL client's shell does not configure the server.
  Keep credentials out of SQL, examples, and committed files.
- The default model is `jev-1.13.0`; `TYPESAFE_MODEL` selects another model.
  Evaluated values and prompts leave the database for TypeSafe. Use synthetic or
  public data for demonstrations; use application data only within the user's
  authorized scope for that transfer.

## Choose valid SQL syntax

Use infix syntax for a column and a fixed, single-quoted prompt:

```sql
SELECT film_id, title
FROM film
WHERE film_id BETWEEN 1 AND 8
  AND description AILIKE 'The story is set somewhere in Asia';
```

Bare, qualified, and backtick-quoted columns work, as does
`column NOT AILIKE 'condition'`. Double apostrophes within prompt literals.

Use `ailike(value, prompt)` for literal values, expressions, or prompts supplied
by another column. All arguments must be strings; cast non-string values with
`CAST(... AS CHAR)`.

```sql
SELECT ailike('A robot helps a child.', 'Includes a robot');

-- An existing join selects candidate pairs; the second column supplies a condition.
SELECT r.id
FROM records AS r
JOIN filter_rules AS f ON f.id = r.rule_id
WHERE ailike(r.description, f.condition_text);
```

Use `ailike(left, right, prompt)` to compare two values. The third argument is the
relationship to evaluate; both earlier arguments are data, not prompts:

```sql
SELECT a.id AS supplier_product_id, b.id AS catalog_product_id
FROM supplier_products AS a
JOIN catalog_products AS b
  ON a.category_id = b.category_id
 AND ailike(
   a.description,
   b.description,
   'Left and right describe the same product type, allowing different wording.'
 );
```

Jev receives the values as separate `left` and `right` fields. Reference those
names in the prompt when the relationship is directional. Preserve their order;
swapping the arguments can change the meaning.

In application queries, bind parameters through the database driver and use
function syntax throughout the statement:

```sql
SELECT film_id, title FROM film
WHERE film_id = ? AND ailike(description, ?);

SELECT a.id, b.id
FROM supplier_products AS a
JOIN catalog_products AS b ON a.category_id = b.category_id
WHERE a.id = ? AND ailike(a.description, b.description, ?);
```

Any real `?` placeholder anywhere in a query containing infix AILIKE prevents
rewriting, including placeholders outside its predicate. Use function syntax
also for queries containing backslash-escaped strings or MySQL executable
comments (`/*! ... */`). Infix remains a one-value form. Do not generate
`column AILIKE other_column`, `column AILIKE ?`, `'literal' AILIKE 'condition'`, or
`left_column AILIKE right_column USING 'relationship'`.

## Design the condition

- Ask one explicit yes/no condition or relationship. Jev sees only the supplied
  value or values and prompt, not the rest of the row or database. Use the
  three-argument form for two pieces of data and a relationship; use `CONCAT(...)`
  when additional context belongs within a value.
- State details that matter: use “in July 2027” when both month and year matter.
  Do not assume ambiguous shorthand such as “July 27” specifies a year.
- The result is `1` when the model's probability is at least `0.5`, otherwise `0`.
  NULL inputs return NULL without an API call, so a NULL probe does not validate
  credentials. This is a model judgment, not guaranteed equality.
  There is no `ailike_score` or configurable threshold; do not invent one. The
  third argument is a relationship prompt, never a threshold. Raising a threshold
  would not resolve an ambiguous condition or recover a missed match.

## Bound execution and handle errors

Each uncached argument tuple makes a synchronous API request. Narrow candidate
rows and joined pairs with ordinary SQL. `LIMIT` on the result does not cap API
calls; when a hard candidate budget matters, materialize a bounded candidate set
before applying AILIKE. A full cross join can evaluate every pair.

The cache includes the value and prompt, or both values and the prompt for
three-argument calls. It is bounded and local to each expression, not shared
across queries.
Defaults are 1,000 uncached requests per expression, a 10-second request timeout,
and a 3-second connection timeout. Server environment settings
`AILIKE_MAX_REQUESTS`, `AILIKE_TIMEOUT_MS`, and `AILIKE_CONNECT_TIMEOUT_MS` adjust
these limits. Each value is limited to 32 KiB and prompts to 8 KiB; empty prompts
are errors. There is no index support, batching, or automatic retry.

API failures and exceeded limits abort the statement. Preserve that error in
application handling instead of converting it into “no matching rows.” Use AILIKE
for reads; do not treat a model match as sufficient grounds for unattended data
modification or statement-based replication.

## Validate the integration

From a repository checkout, use the isolated mock-backed tests without a real API
key or existing database:

```sh
python3 scripts/test.py
MYSQL_TEST_VERSION=8.0.46 python3 scripts/test.py
```

For live behavior, read `docs/sample-data.md` or the
[Sakila demo guide](https://github.com/maayanlevy/mysql-ailike/blob/main/docs/sample-data.md).
`scripts/demo.py` makes real TypeSafe calls and drops/reloads Sakila in this
project's demo container. Never import its sample schema into an unrelated server.

Test a small set of positive, negative, near-match, and NULL cases for the actual
condition before widening a query. Mock tests establish SQL/API behavior; live
examples establish only the observed model behavior on those inputs. Report
validation evidence separately from user-facing usage documentation.
