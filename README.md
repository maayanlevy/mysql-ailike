# AILIKE for MySQL

A native MySQL plugin for filtering rows and comparing text columns with
natural-language conditions, powered by [TypeSafe Jev](https://docs.typesafe.ai).

**[Install and check compatibility](docs/install.md) · [Try the Docker demo](docs/sample-data.md#run-the-local-demo)**

Preview release. Requires a TypeSafe API key; evaluated values and prompts are
sent to TypeSafe.

## Match by meaning

Find stories set in Asia when their descriptions mention China or India:

```sql
SELECT film_id, title, description
FROM sakila.film
WHERE film_id BETWEEN 1 AND 8
  AND description AILIKE 'The story takes place somewhere in Asia';
```

![AILIKE matches three Sakila films set in Ancient China or India.](docs/screenshots/asia.png)

## Syntax

| Syntax | Question |
| --- | --- |
| `column AILIKE 'condition'` | Does this value satisfy the condition? |
| `ailike(value, prompt)` | Does this value satisfy the condition? |
| `ailike(left, right, prompt)` | Do these values satisfy the relationship? |

AILIKE returns `1` for a match and `0` otherwise. Any `NULL` argument returns
`NULL`. Arguments are strings; use `CAST(... AS CHAR)` for other types.

Narrow candidates with ordinary SQL conditions: each uncached evaluation makes
an API request.

## Compare joined columns

The [demo dataset](sql/join-demo.sql) contains three supplier products and three
catalog products. With AILIKE installed, open `mysql -u root -p` from the repository
root and load it into a fresh database:

```sql
CREATE DATABASE ailike_join_demo;
USE ailike_join_demo;
SOURCE sql/join-demo.sql;
```

Pass both columns and describe the relationship:

```sql
SELECT a.id AS supplier_id, a.description AS supplier,
       b.id AS catalog_id, b.description AS catalog
FROM supplier_products AS a
JOIN catalog_products AS b
  ON a.category_id = b.category_id
 AND ailike(
   a.description,
   b.description,
   'Left and right describe the same kind of product, '
   'with matching material and key features. Wording may differ.'
 )
ORDER BY a.id, b.id;
```

Expected matches: `(1, 1)` for the steel bottle and vacuum flask, and `(3, 3)` for
the headphones. The glass carafe and plastic bottle have no matching pair.

![AILIKE joins the steel bottle with the vacuum flask and the two headphone descriptions.](docs/screenshots/join.png)

## More examples

<details>
<summary>Find someone who prepares food</summary>

Match a pastry chef without searching for those exact words:

```sql
SELECT film_id, title, description
FROM sakila.film
WHERE film_id BETWEEN 1 AND 8
  AND description AILIKE 'The story features somebody whose profession is preparing food';
```

![AILIKE matches AFRICAN EGG, whose description mentions a pastry chef.](docs/screenshots/chef.png)

</details>

<details>
<summary>Match dates described in words</summary>

Specify both month and year to avoid ambiguity:

```sql
SELECT rental_id, rental_date
FROM sakila.rental
WHERE customer_id = 1
  AND ailike(CAST(rental_date AS CHAR),
             'In July 2005, in the same calendar month and year.')
ORDER BY rental_id;
```

![AILIKE matches twelve July 2005 rentals for customer 1.](docs/screenshots/dates.png)

AILIKE returns a model judgment. Use SQL date functions for exact comparisons.

</details>

[Contributing](CONTRIBUTING.md) · [Report a vulnerability](SECURITY.md) ·
[AI coding agent skill](skills/mysql-ailike/SKILL.md) · [GPL-2.0-only](LICENSE)
