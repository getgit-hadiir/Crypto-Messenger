# Crypto Messenger 0.1.0 — Installation and Startup Guide

## 1. Requirements

This prototype targets Linux systems with:

- GCC or Clang
- OpenSSL development headers and libraries
- SQLite3 development headers and libraries
- GNU libmicrohttpd development headers and library
- json-c development headers and library
- Make

On Debian/Ubuntu:

```sh
sudo apt update
sudo apt install build-essential pkg-config libssl-dev libsqlite3-dev libmicrohttpd-dev libjson-c-dev
```

## 2. Build

From the project root:

```sh
make
```

The executable is created at `build/crypto-messenger`.

## 3. Start

```sh
./build/crypto-messenger
```

Open this address in a browser:

```text
http://127.0.0.1:8080
```

The server listens on loopback only. It is not exposed to the local network or the Internet.

## 4. Use

1. Choose English or Persian.
2. Enter a message.
3. Enter a strong passphrase and confirm it.
4. Select **Encrypt**.
5. Copy the encrypted text or save it as a `.cmsg` file.
6. To open a received message, paste its encrypted text or load its `.cmsg` file, enter the same passphrase, and select **Decrypt**.
7. Select **Save to database** to store the encrypted payload in `data/messenger.db`.

The recipient must have a compatible Crypto Messenger build and the same passphrase. Share the passphrase separately from the encrypted message.

## 5. Database

The application creates:

```text
data/messenger.db
```

Only encrypted payloads are stored by the Save action. The database is local to this installation.

## 6. Security notes

- AES-256-GCM provides confidentiality and authentication.
- PBKDF2-HMAC-SHA-256 derives an encryption key from the passphrase.
- Each encryption operation generates a fresh random salt and nonce.
- Do not reuse weak passwords.
- Never send the passphrase in the same message as the encrypted payload.
- The development server is intended for local testing only.
- This prototype has not undergone an independent security audit.
- Public-key encryption and attachments are not implemented in 0.1.0.

## 7. Stop

Press `Ctrl+C` in the terminal.

## 8. Troubleshooting

If compilation reports a missing package, install the corresponding development package listed in section 1.

If port 8080 is already in use, stop the other service before starting this prototype.
