#!/usr/bin/env python3
"""Load pinned Sakila and test AILIKE on this repo's isolated live MySQL."""
from pathlib import Path
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
COMPOSE = ['docker', 'compose', '-f', str(ROOT / 'compose.yaml')]


def sql(statement):
    return subprocess.run([*COMPOSE, 'exec', '-T', '-e', 'MYSQL_PWD=local-ailike-only',
                           'mysql', 'mysql', '-uroot', '--batch', '--skip-column-names',
                           '--raw'], input=statement, text=True, capture_output=True,
                          check=True, cwd=ROOT).stdout.strip()


def main():
    subprocess.run([sys.executable, str(ROOT / 'scripts/fetch-sakila.py')], check=True)
    subprocess.run([*COMPOSE, 'up', '--build', '--wait', '--wait-timeout', '180'],
                   cwd=ROOT, check=True)
    for name in ('sakila-schema.sql', 'sakila-data.sql'):
        sql((ROOT / '.local/sakila' / name).read_text())
    assert sql('SELECT COUNT(*) FROM sakila.film') == '1000'
    cases = [
        ('The story is set somewhere in Asia.', ['2', '6', '8']),
        ('The story includes a robot as a character.', ['6']),
        ('The story features somebody whose profession is preparing food.', ['5']),
    ]
    for prompt, expected in cases:
        started = time.monotonic()
        rows = sql("SELECT film_id, title FROM sakila.film "
                   "WHERE film_id BETWEEN 1 AND 8 "
                   f"AND description AILIKE '{prompt}' ORDER BY film_id;")
        ids = [row.split('\t')[0] for row in rows.splitlines()]
        print(f'\n{prompt}\n{rows}\n{time.monotonic() - started:.2f}s', flush=True)
        assert ids == expected, f'Expected {expected}, got {ids}; live model judgments may vary.'
    print('\nPassed 3 live semantic filters across 8 Sakila films each (24 judgments).')


if __name__ == '__main__':
    main()
