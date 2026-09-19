# AILIKE for MySQL

Filter rows with a natural-language condition, powered by [TypeSafe Jev](https://docs.typesafe.ai).

```sql
SELECT *
FROM film
WHERE description AILIKE 'The story takes place somewhere in Asia';
```

AILIKE is a native MySQL plugin that runs inside your existing server.

**[Plugin bundles, installation, and verified compatibility →](docs/install.md)**

## Match dates described in words

Use the function form for literal values:

```sql
SELECT ailike('2027-07-01', 'In July 2027, in the same calendar month and year.');
```

Cast date columns to text. Use ordinary SQL to narrow candidates before applying
the natural-language condition, as in this Sakila example:

```sql
SELECT rental_id, rental_date
FROM sakila.rental
WHERE customer_id = 1
  AND ailike(CAST(rental_date AS CHAR),
             'In July 2005, in the same calendar month and year.')
ORDER BY rental_id;
```

Specify the year when it matters: `same month as July 27` is ambiguous. AILIKE
returns a model judgment; use SQL date functions for exact date comparisons.

## Comparing joined columns

Use `ailike(a.value, b.prompt)` when another column supplies the condition, or
build a condition with `CONCAT(...)`. Infix syntax requires a column on the left
and a literal prompt on the right.
Use SQL normalization and equality for exact identifiers.
Each uncached candidate pair makes an API request, so narrow joins with ordinary
SQL first.

For AI coding agents: [AILIKE usage skill](skills/mysql-ailike/SKILL.md).

[GPL-2.0-only](LICENSE)
