/*
 * File: crypto.h
 * Project: Crypto Messenger
 * Version: 0.1.0
 * Updated: 2026-11-23
 * Description: Password-based authenticated encryption interface.
 */

#ifndef CRYPTO_H
#define CRYPTO_H

#include <stddef.h>

int crypto_encrypt_text(const char *plain, const char *password, char **payload);
int crypto_decrypt_text(const char *payload, const char *password, char **plain);
void crypto_free(void *ptr);

#endif

/*==== End of file crypto.h ====*/
