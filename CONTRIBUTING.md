# Contributing to sqlite-c

Thank you for considering a contribution to sqlite-c. Contributions may include bug fixes, regression tests, documentation improvements, and focused improvements to the SQL parser, storage engine, pager, or B tree.

## Before you start

Check existing issues and pull requests before starting work. Discuss substantial changes to the database file format, page layout, SQL behavior, or storage architecture before implementing them. The file format is project specific and is not compatible with SQLite database files.

## Development setup

The project requires a C compiler and GNU Make or a compatible make implementation. From the repository root, build and run the tests:

```bash
make
make test
```

The build uses C11 and strict compiler warnings, including `-Werror`. Keep new code compatible with the project's compiler settings. Build outputs are written to `build/`.

To run the insertion benchmark:

```bash
make benchmark
```

Benchmark results depend on the compiler, flags, operating system, and machine. Do not present one local result as a general performance guarantee.

## Storage and correctness expectations

Database changes must preserve file and tree invariants, or explicitly document and test an intentional format change.

- Add regression tests for bugs and edge cases.
- Test persistence by closing and reopening a database when a change affects stored data.
- For B tree changes, consider key ordering, leaf links, parent references, separator keys, page capacities, and root behavior.
- For pager or file-format changes, test invalid input, truncated or malformed files, and error paths where relevant.
- Check allocation, file I/O, and close/flush failures rather than assuming they succeed.
- Do not claim compatibility with SQLite's file format or unsupported SQL features.

Where available, run the repository's sanitizer and CI checks. If a sanitizer or compiler is unavailable locally, state that limitation rather than implying the check passed.

## Pull request expectations

A good pull request should:

- Explain the problem and the implementation approach.
- Stay focused and avoid unrelated refactoring.
- Include tests that fail before the fix and pass after it for bug fixes.
- Pass `make test` and the relevant CI checks, or document a reproducible existing failure.
- Update the README when supported behavior, limitations, build steps, or the file format changes.
- Avoid committing build outputs, database files, editor files, or unrelated generated artifacts.
- Respect the project's MIT License and any applicable third-party licenses.

## Reporting bugs

Include steps to reproduce the issue, expected and actual behavior, compiler and operating system details where relevant, and sanitized error output. Do not attach private data or unrelated database contents.

## License

By submitting a contribution, you agree that your contribution is provided under the project's [MIT License](LICENSE). You retain the copyright to your contribution, subject to the license terms.
