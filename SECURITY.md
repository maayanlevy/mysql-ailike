# Security

Report vulnerabilities privately using
[GitHub's vulnerability reporting form](https://github.com/maayanlevy/mysql-ailike/security/advisories/new),
rather than a public issue. Include the AILIKE version, MySQL version, operating
system and architecture, a minimal reproduction, and the expected impact. Remove
API keys, credentials, and private database contents from reports.

Only the latest published release receives security fixes. AILIKE is currently
a preview release.

AILIKE sends evaluated column values and prompts to TypeSafe. Only install it in
servers where that transfer is authorized. API keys belong in a server-side secret
file or environment variable, never SQL. The plugin does not log request contents
or credentials. API failures abort the statement.

The Docker setup is a local development environment with a documented development
password and a localhost-only port. It must not be deployed unchanged as a public
database. MySQL UDFs execute in the server process; install trusted release
bundles or build from trusted source.
