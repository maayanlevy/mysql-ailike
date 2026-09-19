# Sample data

The demo uses [jOOQ's MySQL Sakila sample](https://github.com/jOOQ/sakila/tree/e089a5b1ec9af0df7a9c6a5d47d49fa1736a4e84/mysql-sakila-db), pinned to commit `e089a5b1ec9af0df7a9c6a5d47d49fa1736a4e84`.
Sakila models a DVD rental store with 16 tables and seven views. Its 1,000 films have short plot descriptions suited to semantic filtering. The records are generated sample data, including fictional films and customers.

```sh
python3 scripts/fetch-sakila.py
```

This downloads the schema, data, and upstream license into `.local/sakila/`. The script checks SHA-256 before saving each download and reuses existing files only when their checksums match. It never connects to a database. The SQL schema drops and recreates `sakila`; import it only into the disposable demo server.

| Local file | SHA-256 |
| --- | --- |
| `sakila-schema.sql` | `f2c41c3bf6d6c239941b4f98fb37afad21f6be12b82bf586202529a793ccc2ee` |
| `sakila-data.sql` | `353ef858e4d2d1a60549969283434da15b905aef3ca7099d82b649b75f8de99f` |
| `LICENSE` | `1f1f3467bc7e7ba6277f20ddd7925e2ec2165d3bd56bdaf2eb26ea84c3953279` |

For a small semantic demo, consider film IDs 1 through 8. Descriptions for IDs 2 and 6 place the story in Ancient China; ID 8 places it in Ancient India. The criterion “The story is set somewhere in Asia” should match these three films even though their descriptions do not contain “Asia.” Restricting candidate IDs keeps the number of model calls small.

## Attribution

Sakila was originally developed by MySQL AB. Its [SQL files use the New BSD license](https://dev.mysql.com/doc/sakila/en/sakila-license.html); the downloaded schema includes MySQL AB's 2006 copyright, three conditions, and disclaimer. The [jOOQ repository license](https://github.com/jOOQ/sakila/blob/e089a5b1ec9af0df7a9c6a5d47d49fa1736a4e84/LICENSE) is BSD-2-Clause, copyright 2021 jOOQ Object Oriented Querying. Preserve both notices when redistributing the sample. These upstream licenses apply separately from this project's license.

## Run the local demo

Requires Docker Compose, Python 3, and a TypeSafe API key. This starts a separate
MySQL container with its own volume; it never connects to an existing database.

```sh
mkdir -p .secrets
# Save your API key as one line in .secrets/typesafe_api_key.
chmod 700 .secrets
chmod 644 .secrets/typesafe_api_key  # Mounted file is readable by MySQL; parent is private.
python3 scripts/demo.py
```

The script reloads Sakila only in the project container and checks three prompts
across eight film descriptions (24 live judgments). The full 1,000-film sample
remains available for exploration. Open its SQL shell with:

```sh
docker compose exec -e MYSQL_PWD=local-ailike-only mysql mysql -uroot sakila
```

The development server binds to `127.0.0.1:33316`, with user `root` and password
`local-ailike-only`. Set `MYSQL_PORT` to change the port. Stop it with
`docker compose down`; add `--volumes` to delete this project's sample database.

## Verified result

On 2026-09-19, MySQL 8.4.8 on Linux ARM64 with Jev `jev-1.13.0` returned:

| Condition (films 1–8) | Matching film IDs |
| --- | --- |
| Set somewhere in Asia | 2, 6, 8 |
| Includes a robot character | 6 |
| Includes somebody whose profession is preparing food | 5 |

All 24 judgments agreed with the expected sample labels. Each eight-row query took
about 3.2 seconds in this run. This is a smoke test, not a general accuracy or
performance benchmark.
