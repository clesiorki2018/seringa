/*
 * Copyright 2026 clesiorki
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license information.
 */

#include "auth.h"

#include "esp_timer.h"
#include "esp_log.h"

#include <string.h>
#include <stdlib.h>

/*
 * ============================================================================
 * 🧾 TAG
 * ============================================================================
 */
static const char *TAG = "AUTH";

/*
 * ============================================================================
 * 🔐 CONFIGURAÇÃO
 * ============================================================================
 */

#ifndef SERINGA_PIN_HASH
#error "SERINGA_PIN_HASH deve ser definido no .env ou no ambiente de build"
#endif

/*
 * Tamanho do token.
 */
#define TOKEN_SIZE 32


/*
 * ============================================================================
 * ⏱️ TIMEOUT DA SESSÃO
 * ============================================================================
 *
 * 24 horas.
 *
 * IMPORTANTE:
 *  - sessão longa reduz necessidade de login
 *  - adequada para rede local controlada
 * ============================================================================
 */
#define SESSION_TIMEOUT_US \
    (24ULL * 60ULL * 60ULL * 1000000ULL)


/*
 * ============================================================================
 * 🧠 ESTADO INTERNO
 * ============================================================================
 */
static char g_session_token[TOKEN_SIZE] = {0};

static int64_t g_session_expiry = 0;

/*
 * ============================================================================
 * 🔐 HASH DJB2
 * ============================================================================
 *
 * Hash simples e leve.
 *
 * OBS:
 *  Não é criptografia forte.
 * ============================================================================
 */
static uint32_t auth_simple_hash(
    const char *str
)
{
    uint32_t hash = 5381;

    while (*str) {

        hash =
            ((hash << 5) + hash)
            + (uint8_t)(*str);

        str++;
    }

    return hash;
}

/*
 * ============================================================================
 * 🔐 GERA TOKEN
 * ============================================================================
 */
static void auth_generate_token(void)
{
    static const char charset[] =
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789";

    for (int i = 0; i < TOKEN_SIZE - 1; i++) {

        g_session_token[i] =
            charset[rand() %
            (sizeof(charset) - 1)];
    }

    g_session_token[TOKEN_SIZE - 1] = '\0';
}

/*
 * ============================================================================
 * 🔐 VALIDA AUTORIZAÇÃO
 * ============================================================================
 */
bool auth_is_authorized(
    httpd_req_t *req
)
{
    char buf[64];

    /*
     * ================================================================
     * 📥 HEADER
     * ================================================================
     */
    if (
        httpd_req_get_hdr_value_str(
            req,
            "Authorization",
            buf,
            sizeof(buf)
        ) != ESP_OK
    ) {
        return false;
    }

    /*
     * ================================================================
     * 🔐 TOKEN
     * ================================================================
     */
    if (
        strcmp(
            buf,
            g_session_token
        ) != 0
    ) {
        return false;
    }

    /*
     * ================================================================
     * ⏱️ TIMEOUT
     * ================================================================
     */
    int64_t now =
        esp_timer_get_time();

    if (now > g_session_expiry) {

        ESP_LOGW(
            TAG,
            "Sessão expirada"
        );

        g_session_token[0] = '\0';

        return false;
    }

    /*
     * ================================================================
     * 🔄 RENOVA SESSÃO
     * ================================================================
     */
    g_session_expiry =
        now + SESSION_TIMEOUT_US;

    return true;
}

/*
 * ============================================================================
 * 🔐 LOGIN
 * ============================================================================
 */
esp_err_t auth_login_handler(
    httpd_req_t *req
)
{
    char buf[32];

    /*
     * ================================================================
     * 📥 LEITURA
     * ================================================================
     */
    int len =
        httpd_req_recv(
            req,
            buf,
            sizeof(buf) - 1
        );

    if (len <= 0) {

        return httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Erro leitura"
        );
    }

    buf[len] = '\0';

    /*
     * ================================================================
     * 🔐 HASH
     * ================================================================
     */
    uint32_t hash =
        auth_simple_hash(buf);

    /*
     * ================================================================
     * ❌ PIN INVÁLIDO
     * ================================================================
     */
    if (hash != SERINGA_PIN_HASH) {

        return httpd_resp_send_err(
            req,
            HTTPD_401_UNAUTHORIZED,
            "PIN incorreto"
        );
    }

    /*
     * ================================================================
     * ✅ LOGIN OK
     * ================================================================
     */
    auth_generate_token();

    g_session_expiry =
        esp_timer_get_time()
        + SESSION_TIMEOUT_US;

    httpd_resp_set_type(
        req,
        "text/plain"
    );

    httpd_resp_sendstr(
        req,
        g_session_token
    );

    return ESP_OK;
}
