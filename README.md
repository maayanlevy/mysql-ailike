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

Literal values also use the function form:

```sql
SELECT ailike('2027-07-01', 'In July 2027, in the same calendar month and year.');
-- Returned 1 locally.
```

With the Sakila sample, this filters six candidate rentals down to IDs `5000`
and `10000`:

```sql
SELECT rental_id, rental_date
FROM sakila.rental
WHERE rental_id IN (1, 1000, 3000, 5000, 10000, 15000)
  AND ailike(CAST(rental_date AS CHAR),
             'In July 2005, in the same calendar month and year.')
ORDER BY rental_id;
```

Specify the year explicitly: `same month as July 27` also matched July in other
years. These examples were checked locally with MySQL 8.4.8 and Jev `jev-1.13.0`;
they are model judgments, not guarantees. Use SQL date functions for deterministic
date comparisons.

## Comparing joined columns

Dynamic prompts work with `ailike(a.value, CONCAT('condition ', b.value))`;
infix syntax requires a column on the left and a literal prompt on the right.
Exact ID reconciliation was unreliable in local tests: an explicit prefix-removal
prompt matched `idn-503294825723` to `503294825723`, but repeatedly missed the valid
pair `idn-42` and `42`. Use SQL normalization and equality for exact identifiers.
Each uncached candidate pair makes an API request, so narrow joins with ordinary
SQL first.

[GPL-2.0-only](LICENSE)
