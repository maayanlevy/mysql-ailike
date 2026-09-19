#!/usr/bin/env python3
"""Build and test only this project's disposable MySQL + mock API containers."""
from pathlib import Path
import argparse
import json
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
COMPOSE = ['docker', 'compose', '-f', str(ROOT / 'compose.test.yaml')]


def compose(*args, **kwargs):
    return subprocess.run([*COMPOSE, *args], cwd=ROOT, **kwargs)


def sql(statement, expected=None, error=None):
    result = compose('exec', '-T', '-e', 'MYSQL_PWD=local-ailike-test', 'mysql',
                     'mysql', '-uroot', '--batch', '--skip-column-names', '--raw',
                     'ailike_test', input=statement, text=True, capture_output=True)
    if error:
        assert result.returncode != 0, f'Expected SQL failure: {statement}'
        assert error.lower() in result.stderr.lower(), result.stderr
    else:
        assert result.returncode == 0, (statement, result.stderr)
        if expected is not None:
            assert result.stdout.strip() == expected, (statement, result.stdout, expected)


def main(bundle=None):
    try:
        compose('up', '--no-build' if bundle else '--build', '--wait', '--wait-timeout', '180', check=True)
        sql("CREATE TABLE texts (id INT PRIMARY KEY, content TEXT, prompt TEXT);"
            "INSERT INTO texts VALUES (1,'cat','cat'),(2,'dog','cat'),(3,NULL,'cat');"
            "CREATE TABLE rules (id INT PRIMARY KEY, content TEXT, prompt TEXT);"
            "INSERT INTO rules VALUES (1,'cat','equal'),(2,'dog','equal');"
            "CREATE TABLE pairs (id INT PRIMARY KEY, l TEXT, r TEXT, prompt TEXT);"
            "INSERT INTO pairs VALUES (1,'cat','cat','equal'),(2,'cat','dog','equal'),"
            "(3,'cat','dog','left-before-right'),(4,'dog','cat','left-before-right'),"
            "(5,'ab','a','left-before-right'),(6,'a','ba','left-before-right'),"
            "(7,CONCAT('a',CHAR(0),'z'),'b','left-before-right'),"
            "(8,'a',CONCAT(CHAR(0),'zb'),'left-before-right');")
        checks = [
            ("SELECT id FROM texts WHERE content AILIKE 'cat'", '1'),
            ("SELECT id FROM texts WHERE content NOT AILIKE 'cat'", '2'),
            ("SELECT id FROM texts t WHERE `t`.`content` aIlIkE 'dog'", '2'),
            ("SELECT id FROM texts WHERE ailike(content,prompt)", '1'),
            ("SELECT ailike(NULL,'cat'),ailike('cat',NULL)", 'NULL\tNULL'),
            ("SELECT ailike('boundary','unused')", '1'),
            ("SELECT 'AILIKE in a string',1 /* content AILIKE 'ignored' */", 'AILIKE in a string\t1'),
            ("SELECT ailike('O''Brien','O''Brien')", '1'),
            ("SELECT ailike('שלום 🐈','שלום 🐈')", '1'),
            ("SELECT COUNT(*) FROM texts WHERE id<3 AND content AILIKE 'cat' OR id=3", '2'),
            ("PREPARE s FROM 'SELECT id FROM texts WHERE ailike(content, ?)';"
             "SET @p='cat'; EXECUTE s USING @p; SET @p='dog'; EXECUTE s USING @p; DEALLOCATE PREPARE s;", '1\n2'),
            ("SET sql_mode='ANSI_QUOTES,NO_BACKSLASH_ESCAPES'; SELECT id FROM texts WHERE content AILIKE 'cat'", '1'),
            ("SELECT ailike('cat','cat','equal'),ailike('cat','dog','equal')", '1\t0'),
            ("SELECT t.id,r.id FROM texts t JOIN rules r ON t.id=r.id "
             "AND ailike(t.content,r.content,r.prompt) ORDER BY t.id", '1\t1\n2\t2'),
            ("SELECT id,ailike(l,r,prompt) FROM pairs ORDER BY id", '1\t1\n2\t0\n3\t1\n4\t0\n5\t0\n6\t1\n7\t1\n8\t0'),
            ("SELECT ailike(NULL,'http:401','equal'),ailike('http:401',NULL,'equal'),"
             "ailike('http:401','x',NULL)", 'NULL\tNULL\tNULL'),
            ("SELECT ailike('שלום 🐈','שלום 🐈','שלום 🐈')", '1'),
            ("SET NAMES latin1; SELECT ailike(_latin1 0xe9,_latin1 0xe9,_latin1 0xe9)", '1'),
            ("SELECT ailike('O''Brien','Brien','left-contains-right')", '1'),
            ("SELECT ailike(CONCAT('a',CHAR(0),'b'),CONCAT('a',CHAR(0),'b'),'equal')", '1'),
            ("SELECT ailike('','','equal'),ailike('boundary','x','equal')", '1\t1'),
            ("SELECT ailike(REPEAT('x',32768),REPEAT('x',32768),'equal')", '1'),
            ("SELECT ailike(CAST(12 AS CHAR),CAST(12 AS CHAR),'equal')", '1'),
            ("PREPARE s FROM 'SELECT ailike(?,?,?)'; SET @l='cat',@r='cat',@p='equal';"
             "EXECUTE s USING @l,@r,@p; SET @r='dog'; EXECUTE s USING @l,@r,@p;"
             "SET @p='left-before-right'; EXECUTE s USING @l,@r,@p; DEALLOCATE PREPARE s;", '1\n0\n1'),
        ]
        for statement, expected in checks:
            sql(statement, expected=expected)
        failures = [
            ("PREPARE s FROM 'SELECT id FROM texts WHERE content AILIKE ?'", 'ailike(column, ?)'),
            ("SELECT ailike('http:401','x')", 'HTTP 401'),
            ("SELECT ailike('http:429','x')", 'HTTP 429'),
            ("SELECT ailike('http:529','x')", 'HTTP 529'),
            ("SELECT ailike('malformed','x')", 'invalid'),
            ("SELECT ailike('wrong-type','x')", 'invalid'),
            ("SELECT ailike('out-of-range','x')", 'outside'),
            ("SELECT ailike('slow','x')", 'timed out'),
            ("SELECT ailike('cat','')", 'empty'),
            ("SELECT ailike(REPEAT('x',32769),'x')", '32768'),
            ("SELECT ailike('x',REPEAT('x',8193))", '8192'),
            ("SELECT id FROM texts WHERE UPPER(content) AILIKE 'CAT'", 'AILIKE'),
            ("SELECT ailike('cat')", 'ailike'),
            ("SELECT ailike('a','b','equal','extra')", '2 or 3'),
            ("SELECT ailike(1,'1','equal')", 'strings'),
            ("SELECT ailike('1',1,'equal')", 'strings'),
            ("SELECT ailike('1','1',1)", 'strings'),
            ("SELECT ailike('cat','cat','')", 'empty'),
            ("SELECT ailike(REPEAT('x',32769),'x','equal')", '32768'),
            ("SELECT ailike('x',REPEAT('x',32769),'equal')", '32768'),
            ("SELECT ailike('x','x',REPEAT('x',8193))", '8192'),
            ("SELECT ailike('http:429','x','equal')", 'HTTP 429'),
            ("SELECT ailike('wrong-type','x','equal')", 'invalid'),
        ]
        for statement, error in failures:
            sql(statement, error=error)
        sql("WITH RECURSIVE n AS (SELECT 1 AS i UNION ALL SELECT i+1 FROM n WHERE i<9) "
            "SELECT SUM(ailike(CAST(i AS CHAR),'x')) FROM n", error='MAX_REQUESTS')
        sql("WITH RECURSIVE n AS (SELECT 1 AS i UNION ALL SELECT i+1 FROM n WHERE i<20) "
            "SELECT SUM(ailike(IF(i>0,'cat','dog'),'cat')) FROM n", expected='20')
        sql("WITH RECURSIVE n AS (SELECT 1 AS i UNION ALL SELECT i+1 FROM n WHERE i<20) "
            "SELECT SUM(ailike(IF(i>0,'cat','dog'),'cat','equal')) FROM n", expected='20')
        sql("WITH RECURSIVE n AS (SELECT 1 AS i UNION ALL SELECT i+1 FROM n WHERE i<9) "
            "SELECT SUM(ailike('x',CAST(i AS CHAR),'equal')) FROM n", error='MAX_REQUESTS')
        sql("WITH RECURSIVE n AS (SELECT 1 AS i UNION ALL SELECT i+1 FROM n WHERE i<9) "
            "SELECT SUM(ailike('x','x',CAST(i AS CHAR))) FROM n", error='MAX_REQUESTS')
        sql(((bundle or ROOT / 'sql') / 'uninstall.sql').read_text())
        sql("SELECT COUNT(*) FROM mysql.func WHERE name='ailike'", expected='0')
        sql("SELECT COUNT(*) FROM INFORMATION_SCHEMA.PLUGINS "
            "WHERE PLUGIN_NAME='ailike_rewrite'", expected='0')
        sql(((bundle or ROOT / 'sql') / 'install.sql').read_text())
        sql("SELECT id FROM texts WHERE content AILIKE 'cat'", expected='1')
        sql("SELECT ailike('cat','cat','equal')", expected='1')
        compose('restart', '--no-deps', 'mysql', check=True)
        compose('up', '--no-build', '--no-recreate', '--wait', '--wait-timeout', '180', check=True)
        sql('SELECT COUNT(*) FROM texts', expected='3')
        sql("SELECT PLUGIN_STATUS FROM INFORMATION_SCHEMA.PLUGINS "
            "WHERE PLUGIN_NAME='ailike_rewrite'", expected='ACTIVE')
        sql("SELECT id FROM texts WHERE content AILIKE 'cat'", expected='1')
        sql("SELECT ailike('cat','dog','equal')", expected='0')
        sql('SELECT 1', expected='1')
        print(f'Passed {len(checks) + len(failures) + 16} MySQL integration checks.')
    finally:
        compose('down', '--volumes', check=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bundle', type=Path,
                        help='Test an extracted release bundle on stock MySQL without building')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='ailike-test-') as temporary:
        bundle = args.bundle.resolve() if args.bundle else None
        if bundle:
            for name in ('ailike_udf.so', 'ailike_rewrite.so', 'install.sql', 'uninstall.sql'):
                if not (bundle / name).is_file():
                    parser.error(f'Bundle is missing {name}')
            version = os.environ.get('MYSQL_TEST_VERSION', '8.4.8')
            override = Path(temporary) / 'compose.json'
            override.write_text(json.dumps({'services': {'mysql': {
                'image': f'mysql:{version}',
                'volumes': [
                    f'{bundle}/ailike_udf.so:/usr/lib64/mysql/plugin/ailike_udf.so:ro',
                    f'{bundle}/ailike_rewrite.so:/usr/lib64/mysql/plugin/ailike_rewrite.so:ro',
                    f'{bundle}/install.sql:/docker-entrypoint-initdb.d/10-ailike.sql:ro',
                ],
            }}}))
            COMPOSE.extend(['-f', str(override)])
        main(bundle)
