# Crypto Messenger 0.1.0

A local-first prototype for encrypting short text messages and saving encrypted payloads in SQLite.

## Current prototype scope

- C backend using OpenSSL, SQLite3, and GNU libmicrohttpd.
- Browser-based interface.
- English and Persian UI strings in separate locale files.
- Password-based authenticated encryption (AES-256-GCM).
- Text payload format with a versioned `CMSG` envelope.
- Save encrypted payloads in one SQLite database file.
- Export/import the same payload as a `.cmsg` file.
- File header validation before decryption.

## Important scope note

This 0.1.0 prototype implements the password-based workflow. Public-key encryption, contact/key management, and binary attachments are defined as next milestones and are not yet enabled in this build. The UI and database layout are intentionally kept extensible for them.

This is a prototype, not a security-audited messenger. Do not use it for highly sensitive information. Use a long, unique passphrase and share it with the recipient through a separate trusted channel.

## Build

See `docs/INSTALL_EN.md` or `docs/INSTALL_FA.md`.

## Run

After building:

```sh
./build/crypto-messenger
```

Then open `http://127.0.0.1:8080` in a browser.

The database is created at `data/messenger.db`.

## Project layout

```text
src/inc/       C headers
src/core/      HTTP server and application logic
src/database/  SQLite persistence
web/           HTML, CSS, JavaScript, locale files
docs/          English and Persian manuals
data/          Runtime database directory
```

All source-code comments, identifiers, and technical documentation inside source files are in English.
