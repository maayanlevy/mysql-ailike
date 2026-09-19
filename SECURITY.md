# Security

Send vulnerability reports privately through GitHub's security advisory workflow.
Do not include API keys, credentials, or private database contents in issues.

AILIKE sends evaluated column values and prompts to TypeSafe. Only install it in
servers where that transfer is authorized. API keys belong in a server-side secret
file or environment variable, never SQL. The plugin does not log request contents
or credentials. API failures abort the statement.

The Docker setup is a local development environment with a documented development
password and a localhost-only port. It must not be deployed unchanged as a public
database. MySQL UDFs execute in the server process; build from trusted source.
