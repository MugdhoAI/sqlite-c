#!/bin/sh
set -eu

database="build/sqlite.db"
rm -f "$database"

output=$(
    {
        printf "INSERT INTO users VALUES (1, 'alice', 'alice@example.com');"
        printf '%600s' ''
        printf '\nSELECT * FROM users;\n.exit\n'
    } | ./build/sqlite-c
)

printf '%s\n' "$output" | grep -q "command too long"
if printf '%s\n' "$output" | grep -q "1 | alice | alice@example.com"; then
    echo "oversized command was partially executed" >&2
    exit 1
fi

rm -f "$database"
output=$(printf "INSERT INTO users VALUES (1, 'alice', 'alice@example.com');\nSELECT * FROM users;\n.exit\n" | ./build/sqlite-c)
printf '%s\n' "$output" | grep -q "1 | alice | alice@example.com"

rm -f "$database"
