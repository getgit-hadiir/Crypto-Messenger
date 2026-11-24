/*
 * File: crypto.c
 * Project: Crypto Messenger
 * Version: 0.1.0
 * Updated: 2026-11-23
 * Description: Implements versioned password-based AES-256-GCM encryption.
 */

#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "crypto.h"

#define SALT_LEN 16
#define NONCE_LEN 12
#define KEY_LEN 32
#define TAG_LEN 16
#define PBKDF2_ITERATIONS 600000
#define MAX_PAYLOAD_BYTES (8 * 1024 * 1024)

static const char b64_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

static char *base64_encode(const unsigned char *data, size_t len)
{
    size_t out_len = (len / 3) * 4 + (len % 3 ? len % 3 + 1 : 0);
    char *out = malloc(out_len + 1);
    if (!out) return NULL;

    size_t i = 0, j = 0;
    while (i < len) {
        size_t remaining = len - i;
        uint32_t a = data[i++];
        uint32_t b = remaining > 1 ? data[i++] : 0;
        uint32_t c = remaining > 2 ? data[i++] : 0;
        uint32_t triple = (a << 16) | (b << 8) | c;

        out[j++] = b64_table[(triple >> 18) & 63];
        out[j++] = b64_table[(triple >> 12) & 63];
        if (remaining > 1) out[j++] = b64_table[(triple >> 6) & 63];
        if (remaining > 2) out[j++] = b64_table[triple & 63];
    }

    out[j] = '\0';
    return out;
}

static int b64_value(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '-') return 62;
    if (c == '_') return 63;
    return -1;
}

static unsigned char *base64_decode(const char *text, size_t *out_len)
{
    size_t len = strlen(text);
    if (len == 0 || len > MAX_PAYLOAD_BYTES || len % 4 == 1) return NULL;

    size_t cap = (len * 3) / 4 + 3;
    unsigned char *out = malloc(cap);
    if (!out) return NULL;

    size_t i = 0, j = 0;
    while (i < len) {
        size_t remaining = len - i;
        if (remaining < 2) {
            free(out);
            return NULL;
        }

        int a = b64_value(text[i++]);
        int b = b64_value(text[i++]);
        int c = remaining > 2 ? b64_value(text[i++]) : -1;
        int d = remaining > 3 ? b64_value(text[i++]) : -1;

        if (a < 0 || b < 0 || (remaining > 2 && c < 0) ||
            (remaining > 3 && d < 0)) {
            free(out);
            return NULL;
        }

        out[j++] = (unsigned char)((a << 2) | (b >> 4));
        if (remaining > 2) out[j++] = (unsigned char)((b << 4) | (c >> 2));
        if (remaining > 3) out[j++] = (unsigned char)((c << 6) | d);
    }

    *out_len = j;
    return out;
}

void crypto_free(void *ptr)
{
    if (ptr) {
        OPENSSL_cleanse(ptr, strlen((char *)ptr));
        free(ptr);
    }
}

int crypto_encrypt_text(const char *plain, const char *password, char **payload)
{
    if (!plain || !password || !payload || strlen(password) < 8) return -1;

    size_t plain_len = strlen(plain);
    if (plain_len > 1024 * 1024) return -1;

    unsigned char salt[SALT_LEN], nonce[NONCE_LEN], key[KEY_LEN];
    if (RAND_bytes(salt, sizeof(salt)) != 1 ||
        RAND_bytes(nonce, sizeof(nonce)) != 1) return -1;

    if (PKCS5_PBKDF2_HMAC(password, (int)strlen(password), salt, SALT_LEN,
                          PBKDF2_ITERATIONS, EVP_sha256(), KEY_LEN, key) != 1)
        return -1;

    size_t raw_len = 4 + 1 + 4 + SALT_LEN + NONCE_LEN + plain_len + TAG_LEN;
    unsigned char *raw = calloc(1, raw_len);
    if (!raw) {
        OPENSSL_cleanse(key, sizeof(key));
        return -1;
    }

    memcpy(raw, "CMSG", 4);
    raw[4] = 1;
    raw[5] = (PBKDF2_ITERATIONS >> 24) & 0xff;
    raw[6] = (PBKDF2_ITERATIONS >> 16) & 0xff;
    raw[7] = (PBKDF2_ITERATIONS >> 8) & 0xff;
    raw[8] = PBKDF2_ITERATIONS & 0xff;
    memcpy(raw + 9, salt, SALT_LEN);
    memcpy(raw + 9 + SALT_LEN, nonce, NONCE_LEN);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int out1 = 0, out2 = 0;
    int ok = ctx &&
        EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, NONCE_LEN, NULL) == 1 &&
        EVP_EncryptInit_ex(ctx, NULL, NULL, key, nonce) == 1 &&
        EVP_EncryptUpdate(ctx, raw + 9 + SALT_LEN + NONCE_LEN, &out1,
                          (const unsigned char *)plain, (int)plain_len) == 1 &&
        EVP_EncryptFinal_ex(ctx, raw + 9 + SALT_LEN + NONCE_LEN + out1, &out2) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_LEN,
                            raw + 9 + SALT_LEN + NONCE_LEN + plain_len) == 1;

    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(key, sizeof(key));

    if (!ok) {
        OPENSSL_cleanse(raw, raw_len);
        free(raw);
        return -1;
    }

    char *encoded = base64_encode(raw, raw_len);
    OPENSSL_cleanse(raw, raw_len);
    free(raw);
    if (!encoded) return -1;

    size_t total = strlen(encoded) + 7;
    *payload = malloc(total);
    if (!*payload) {
        free(encoded);
        return -1;
    }
    snprintf(*payload, total, "CMSG1:%s", encoded);
    free(encoded);
    return 0;
}

int crypto_decrypt_text(const char *payload, const char *password, char **plain)
{
    if (!payload || !password || !plain || strncmp(payload, "CMSG1:", 6) != 0)
        return -1;

    size_t raw_len = 0;
    unsigned char *raw = base64_decode(payload + 6, &raw_len);
    const size_t header_len = 9 + SALT_LEN + NONCE_LEN;
    if (!raw || raw_len < header_len + TAG_LEN || memcmp(raw, "CMSG", 4) != 0 ||
        raw[4] != 1) {
        free(raw);
        return -1;
    }

    uint32_t iterations = ((uint32_t)raw[5] << 24) | ((uint32_t)raw[6] << 16) |
                          ((uint32_t)raw[7] << 8) | raw[8];
    if (iterations < 100000 || iterations > 2000000) {
        free(raw);
        return -1;
    }

    const unsigned char *salt = raw + 9;
    const unsigned char *nonce = raw + 9 + SALT_LEN;
    size_t cipher_len = raw_len - header_len - TAG_LEN;
    const unsigned char *cipher = raw + header_len;
    const unsigned char *tag = raw + header_len + cipher_len;

    unsigned char key[KEY_LEN];
    if (PKCS5_PBKDF2_HMAC(password, (int)strlen(password), salt, SALT_LEN,
                          (int)iterations, EVP_sha256(), KEY_LEN, key) != 1) {
        free(raw);
        return -1;
    }

    char *out = calloc(1, cipher_len + 1);
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int out1 = 0, out2 = 0;
    int ok = out && ctx &&
        EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, NONCE_LEN, NULL) == 1 &&
        EVP_DecryptInit_ex(ctx, NULL, NULL, key, nonce) == 1 &&
        EVP_DecryptUpdate(ctx, (unsigned char *)out, &out1, cipher, (int)cipher_len) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_LEN, (void *)tag) == 1 &&
        EVP_DecryptFinal_ex(ctx, (unsigned char *)out + out1, &out2) == 1;

    EVP_CIPHER_CTX_free(ctx);
    OPENSSL_cleanse(key, sizeof(key));
    OPENSSL_cleanse(raw, raw_len);
    free(raw);

    if (!ok) {
        if (out) {
            OPENSSL_cleanse(out, cipher_len + 1);
            free(out);
        }
        return -1;
    }

    *plain = out;
    return 0;
}

/*==== End of file crypto.c ====*/
