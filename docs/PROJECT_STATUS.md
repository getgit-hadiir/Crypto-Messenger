# Project Status — 0.1.0

## Implemented
- Modular C source layout.
- Local web interface.
- English/Persian resource files.
- Password-based AES-256-GCM message encryption.
- PBKDF2-HMAC-SHA-256 key derivation.
- Versioned binary envelope (`CMSG`, format version 1).
- Base64url text transport representation (`CMSG1:` prefix).
- SQLite persistence of encrypted payloads.
- `.cmsg` text-file export and import.
- Basic input-size and file-format checks.

## Not implemented in this build
- Public-key encryption and recipient key pairs.
- Attachments (images, audio, video, arbitrary files).
- Network delivery, contact discovery, accounts, or synchronization.
- Full database browsing and message-history UI.
- Independent security audit.

## Compatibility policy
The binary envelope contains a format marker and version byte. The decryptor rejects unknown format versions rather than attempting unsafe interpretation. Application version and cryptographic protocol version are separate concepts and should be added to the envelope in a future format revision before public-key and attachment support is released.

## Known prototype limitations
- Local HTTP only; do not expose this server to a network.
- Password minimum is currently 8 characters for usability testing; production policy should require a substantially stronger passphrase.
- Payload transport is Base64url text, so it is larger than the plaintext.
- SQLite stores encrypted payloads only; metadata minimization and database-at-rest protection require further design.
- No rate limiting or authentication is provided because the server binds to loopback only.
