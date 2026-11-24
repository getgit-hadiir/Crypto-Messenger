/*
 * File: http_server.c
 * Project: Crypto Messenger
 * Version: 0.1.0
 * Updated: 2026-11-23
 * Description: Serves the local web interface and JSON API.
 */

#define _GNU_SOURCE
#include <microhttpd.h>
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "app.h"
#include "crypto.h"
#include "db.h"
#include "http_server.h"

#define POST_LIMIT (2 * 1024 * 1024)

struct request_state {
    char *body;
    size_t size;
};

static const char *mime_type(const char *path)
{
    const char *ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    if (strcmp(ext, ".html") == 0) return "text/html; charset=utf-8";
    if (strcmp(ext, ".css") == 0) return "text/css; charset=utf-8";
    if (strcmp(ext, ".js") == 0) return "application/javascript; charset=utf-8";
    if (strcmp(ext, ".txt") == 0) return "text/plain; charset=utf-8";
    return "application/octet-stream";
}

static enum MHD_Result send_response(struct MHD_Connection *connection,
                                     unsigned int status, const char *body,
                                     const char *content_type)
{
    struct MHD_Response *response =
        MHD_create_response_from_buffer(strlen(body), (void *)body, MHD_RESPMEM_MUST_COPY);
    if (!response) return MHD_NO;
    MHD_add_response_header(response, "Content-Type", content_type);
    MHD_add_response_header(response, "Cache-Control", "no-store");
    MHD_add_response_header(response, "X-Content-Type-Options", "nosniff");
    MHD_add_response_header(response, "Content-Security-Policy",
                            "default-src 'self'; style-src 'self'; script-src 'self'; connect-src 'self'");
    enum MHD_Result result = MHD_queue_response(connection, status, response);
    MHD_destroy_response(response);
    return result;
}

static enum MHD_Result send_json(struct MHD_Connection *connection,
                                 unsigned int status, struct json_object *obj)
{
    const char *text = json_object_to_json_string_ext(obj, JSON_C_TO_STRING_PLAIN);
    return send_response(connection, status, text, "application/json; charset=utf-8");
}

static enum MHD_Result serve_static(struct MHD_Connection *connection, const char *url)
{
    const char *relative = strcmp(url, "/") == 0 ? "web/index.html" : NULL;
    char path[512];
    if (!relative) {
        if (strncmp(url, "/web/", 5) == 0) {
            snprintf(path, sizeof(path), "%s", url + 1);
            relative = path;
        } else if (strncmp(url, "/locales/", 9) == 0) {
            snprintf(path, sizeof(path), "web%s", url);
            relative = path;
        } else {
            return send_response(connection, MHD_HTTP_NOT_FOUND, "Not found", "text/plain");
        }
    }

    FILE *fp = fopen(relative, "rb");
    if (!fp) return send_response(connection, MHD_HTTP_NOT_FOUND, "Not found", "text/plain");
    fseek(fp, 0, SEEK_END);
    long length = ftell(fp);
    rewind(fp);
    if (length < 0 || length > 2 * 1024 * 1024) {
        fclose(fp);
        return send_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "File error", "text/plain");
    }
    char *buffer = malloc((size_t)length + 1);
    if (!buffer) {
        fclose(fp);
        return send_response(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, "Memory error", "text/plain");
    }
    size_t read_count = fread(buffer, 1, (size_t)length, fp);
    fclose(fp);
    buffer[read_count] = '\0';
    struct MHD_Response *response =
        MHD_create_response_from_buffer(read_count, buffer, MHD_RESPMEM_MUST_FREE);
    if (!response) {
        free(buffer);
        return MHD_NO;
    }
    MHD_add_response_header(response, "Content-Type", mime_type(relative));
    MHD_add_response_header(response, "Cache-Control", "no-store");
    enum MHD_Result result = MHD_queue_response(connection, MHD_HTTP_OK, response);
    MHD_destroy_response(response);
    return result;
}

static enum MHD_Result handle_api(struct MHD_Connection *connection,
                                  const char *url, const char *body)
{
    struct json_object *request = json_tokener_parse(body ? body : "{}");
    if (!request) {
        struct json_object *err = json_object_new_object();
        json_object_object_add(err, "error", json_object_new_string("Invalid JSON."));
        enum MHD_Result r = send_json(connection, MHD_HTTP_BAD_REQUEST, err);
        json_object_put(err);
        return r;
    }

    struct json_object *result = json_object_new_object();
    enum MHD_Result response_result = MHD_NO;

    if (strcmp(url, "/api/health") == 0) {
        json_object_object_add(result, "status", json_object_new_string("ok"));
        json_object_object_add(result, "version", json_object_new_string(APP_VERSION));
        response_result = send_json(connection, MHD_HTTP_OK, result);
    } else if (strcmp(url, "/api/encrypt") == 0) {
        struct json_object *text_obj = NULL, *pass_obj = NULL;
        json_object_object_get_ex(request, "text", &text_obj);
        json_object_object_get_ex(request, "password", &pass_obj);
        const char *text = text_obj ? json_object_get_string(text_obj) : NULL;
        const char *password = pass_obj ? json_object_get_string(pass_obj) : NULL;
        char *payload = NULL;

        if (!text || !password || strlen(password) < 8 ||
            crypto_encrypt_text(text, password, &payload) != 0) {
            json_object_object_add(result, "error",
                json_object_new_string("Encryption failed. Use a passphrase of at least 8 characters."));
            response_result = send_json(connection, MHD_HTTP_BAD_REQUEST, result);
        } else {
            json_object_object_add(result, "payload", json_object_new_string(payload));
            response_result = send_json(connection, MHD_HTTP_OK, result);
            crypto_free(payload);
        }
    } else if (strcmp(url, "/api/decrypt") == 0) {
        struct json_object *payload_obj = NULL, *pass_obj = NULL;
        json_object_object_get_ex(request, "payload", &payload_obj);
        json_object_object_get_ex(request, "password", &pass_obj);
        const char *payload = payload_obj ? json_object_get_string(payload_obj) : NULL;
        const char *password = pass_obj ? json_object_get_string(pass_obj) : NULL;
        char *plain = NULL;

        if (!payload || !password || crypto_decrypt_text(payload, password, &plain) != 0) {
            json_object_object_add(result, "error",
                json_object_new_string("Unable to decrypt. Check the passphrase and file format."));
            response_result = send_json(connection, MHD_HTTP_BAD_REQUEST, result);
        } else {
            json_object_object_add(result, "text", json_object_new_string(plain));
            response_result = send_json(connection, MHD_HTTP_OK, result);
            crypto_free(plain);
        }
    } else if (strcmp(url, "/api/save") == 0) {
        struct json_object *payload_obj = NULL;
        json_object_object_get_ex(request, "payload", &payload_obj);
        const char *payload = payload_obj ? json_object_get_string(payload_obj) : NULL;
        if (!payload || db_save_payload(payload) != 0) {
            json_object_object_add(result, "error", json_object_new_string("Database save failed."));
            response_result = send_json(connection, MHD_HTTP_INTERNAL_SERVER_ERROR, result);
        } else {
            json_object_object_add(result, "saved", json_object_new_boolean(1));
            response_result = send_json(connection, MHD_HTTP_OK, result);
        }
    } else {
        json_object_object_add(result, "error", json_object_new_string("Unknown API endpoint."));
        response_result = send_json(connection, MHD_HTTP_NOT_FOUND, result);
    }

    json_object_put(result);
    json_object_put(request);
    return response_result;
}

static enum MHD_Result access_handler(void *cls, struct MHD_Connection *connection,
                                      const char *url, const char *method,
                                      const char *version, const char *upload_data,
                                      size_t *upload_data_size, void **con_cls)
{
    (void)cls;
    (void)version;

    if (*con_cls == NULL) {
        struct request_state *state = calloc(1, sizeof(*state));
        if (!state) return MHD_NO;
        *con_cls = state;
        return MHD_YES;
    }

    struct request_state *state = *con_cls;
    if (strcmp(method, "POST") == 0) {
        if (*upload_data_size > 0) {
            if (state->size + *upload_data_size > POST_LIMIT) return MHD_NO;
            char *next = realloc(state->body, state->size + *upload_data_size + 1);
            if (!next) return MHD_NO;
            state->body = next;
            memcpy(state->body + state->size, upload_data, *upload_data_size);
            state->size += *upload_data_size;
            state->body[state->size] = '\0';
            *upload_data_size = 0;
            return MHD_YES;
        }
        return handle_api(connection, url, state->body ? state->body : "{}");
    }

    if (strcmp(method, "GET") == 0) {
        if (strncmp(url, "/api/", 5) == 0)
            return handle_api(connection, url, "{}");
        return serve_static(connection, url);
    }

    return send_response(connection, MHD_HTTP_METHOD_NOT_ALLOWED,
                         "Method not allowed", "text/plain");
}

static void request_completed(void *cls, struct MHD_Connection *connection,
                              void **con_cls, enum MHD_RequestTerminationCode toe)
{
    (void)cls;
    (void)connection;
    (void)toe;
    struct request_state *state = *con_cls;
    if (state) {
        free(state->body);
        free(state);
        *con_cls = NULL;
    }
}

int http_server_run(void)
{
    struct sockaddr_in bind_address;
    memset(&bind_address, 0, sizeof(bind_address));
    bind_address.sin_family = AF_INET;
    bind_address.sin_port = htons(APP_PORT);
    bind_address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

    struct MHD_Daemon *daemon = MHD_start_daemon(
        MHD_USE_INTERNAL_POLLING_THREAD, APP_PORT, NULL, NULL,
        &access_handler, NULL,
        MHD_OPTION_SOCK_ADDR, (struct sockaddr *)&bind_address,
        MHD_OPTION_NOTIFY_COMPLETED, request_completed, NULL,
        MHD_OPTION_CONNECTION_LIMIT, (unsigned int)16,
        MHD_OPTION_END);

    if (!daemon) {
        fprintf(stderr, "Unable to start HTTP server.\n");
        return -1;
    }

    printf("Press Ctrl+C to stop the server.\n");
    for (;;) pause();

    MHD_stop_daemon(daemon);
    return 0;
}

/*==== End of file http_server.c ====*/
