# AILIKE for MySQL

Filter rows with a natural-language condition, powered by [TypeSafe Jev](https://docs.typesafe.ai).

```sql
SELECT *
FROM film
WHERE description AILIKE 'The story takes place somewhere in Asia';
```

AILIKE is a native MySQL plugin that runs inside your existing server.

**[Plugin bundles, installation, and verified compatibility →](docs/install.md)**

[GPL-2.0-only](LICENSE)
