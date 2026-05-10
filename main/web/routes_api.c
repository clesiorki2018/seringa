#include "routes_api.h"

#include "auth.h"

#include "seringa/seringa.h"
#include "motor/motor.h"

#include "esp_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * ============================================================================
 * 🧾 TAG
 * ============================================================================
 */
static const char *TAG = "ROUTES_API";

/*
 * ============================================================================
 * ⚙️ CONFIGURAÇÃO PADRÃO
 * ============================================================================
 *
 * TODO:
 *  substituir por parâmetros vindos da UI
 */
#define DEFAULT_ML 1.0f

/*
 * ============================================================================
 * 📊 STATUS -> STRING
 * ============================================================================
 */
static const char *status_to_str(
    seringa_status_t st
)
{
    switch (st) {

        case SERINGA_CHEIA:
            return "CHEIA";

        case SERINGA_VAZIA:
            return "VAZIA";

        case SERINGA_MOVING:
            return "MOVING";

        case SERINGA_ERROR:
            return "ERROR";

        case SERINGA_IDLE:
        default:
            return "IDLE";
    }
}

/*
 * ============================================================================
 * 📊 STATUS JSON
 * ============================================================================
 */
static esp_err_t api_status_handler(
    httpd_req_t *req
)
{
    /*
     * ================================================================
     * 🔐 AUTORIZAÇÃO
     * ================================================================
     */
    if (!auth_is_authorized(req)) {

        return httpd_resp_send_err(
            req,
            HTTPD_401_UNAUTHORIZED,
            "Sessão inválida"
        );
    }

    httpd_resp_set_type(
        req,
        "application/json"
    );

    /*
     * ================================================================
     * 📊 STATUS
     * ================================================================
     */
    seringa_status_t st =
        seringa_get_status();

    char json[256];

    snprintf(
        json,
        sizeof(json),

        "{"
        "\"seringa\":\"%s\","
        "\"motor\":\"%s\","
        "\"busy\":%s,"
        "\"cheia\":%s,"
        "\"vazia\":%s"
        "}",

        status_to_str(st),

        motor_is_running()
            ? "RUNNING"
            : "IDLE",

        seringa_is_busy()
            ? "true"
            : "false",

        seringa_is_cheia()
            ? "true"
            : "false",

        seringa_is_vazia()
            ? "true"
            : "false"
    );

    return httpd_resp_sendstr(
        req,
        json
    );
}

/*
 * ============================================================================
 * 💉 INJETAR
 * ============================================================================
 */
static esp_err_t api_inc_handler(
    httpd_req_t *req
)
{
    if (!auth_is_authorized(req)) {

        return httpd_resp_send_err(
            req,
            HTTPD_401_UNAUTHORIZED,
            "Sessão inválida"
        );
    }

    /*
     * ================================================================
     * 🚫 OCUPADO
     * ================================================================
     */
    if (seringa_is_busy()) {

        return httpd_resp_sendstr(
            req,
            "BUSY"
        );
    }

    /*
     * ================================================================
     * 💉 EXECUTA
     * ================================================================
     */
    bool ok =
        seringa_injetar_ml(
            DEFAULT_ML,
            SERINGA_FLOW_SMOOTH
        );

    if (!ok) {

        return httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Falha movimento"
        );
    }

    return httpd_resp_sendstr(
        req,
        "OK"
    );
}

/*
 * ============================================================================
 * ♻️ RECARREGAR
 * ============================================================================
 */
static esp_err_t api_dec_handler(
    httpd_req_t *req
)
{
    if (!auth_is_authorized(req)) {

        return httpd_resp_send_err(
            req,
            HTTPD_401_UNAUTHORIZED,
            "Sessão inválida"
        );
    }

    if (seringa_is_busy()) {

        return httpd_resp_sendstr(
            req,
            "BUSY"
        );
    }

    bool ok =
        seringa_recarregar_ml(
            DEFAULT_ML,
            SERINGA_FLOW_NORMAL
        );

    if (!ok) {

        return httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Falha movimento"
        );
    }

    return httpd_resp_sendstr(
        req,
        "OK"
    );
}

/*
 * ============================================================================
 * 🧪 ENCHIMENTO TOTAL
 * ============================================================================
 */
static esp_err_t api_fill_handler(
    httpd_req_t *req
)
{
    if (!auth_is_authorized(req)) {

        return httpd_resp_send_err(
            req,
            HTTPD_401_UNAUTHORIZED,
            "Sessão inválida"
        );
    }

    if (seringa_is_busy()) {

        return httpd_resp_sendstr(
            req,
            "BUSY"
        );
    }

    bool ok =
        seringa_encher_total();

    if (!ok) {

        return httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Falha enchimento"
        );
    }

    return httpd_resp_sendstr(
        req,
        "OK"
    );
}

/*
 * ============================================================================
 * 🛑 STOP
 * ============================================================================
 */
static esp_err_t api_stop_handler(
    httpd_req_t *req
)
{
    if (!auth_is_authorized(req)) {

        return httpd_resp_send_err(
            req,
            HTTPD_401_UNAUTHORIZED,
            "Sessão inválida"
        );
    }

    seringa_stop();

    return httpd_resp_sendstr(
        req,
        "OK"
    );
}

/*
 * ============================================================================
 * 🧠 REGISTRO DE ROTAS API
 * ============================================================================
 */
esp_err_t register_api_routes(
    httpd_handle_t server
)
{
    httpd_uri_t routes[] = {

        /*
         * ============================================================
         * 🔐 LOGIN
         * ============================================================
         */
        {
            "/api/login",
            HTTP_POST,
            auth_login_handler,
            NULL
        },

        /*
         * ============================================================
         * 📊 STATUS
         * ============================================================
         */
        {
            "/api/status",
            HTTP_GET,
            api_status_handler,
            NULL
        },

        /*
         * ============================================================
         * 💉 INJETAR
         * ============================================================
         */
        {
            "/api/inc",
            HTTP_GET,
            api_inc_handler,
            NULL
        },

        /*
         * ============================================================
         * ♻️ RECARREGAR
         * ============================================================
         */
        {
            "/api/dec",
            HTTP_GET,
            api_dec_handler,
            NULL
        },

        /*
         * ============================================================
         * 🧪 ENCHER TOTAL
         * ============================================================
         */
        {
            "/api/fill",
            HTTP_GET,
            api_fill_handler,
            NULL
        },

        /*
         * ============================================================
         * 🛑 STOP
         * ============================================================
         */
        {
            "/api/stop",
            HTTP_GET,
            api_stop_handler,
            NULL
        }
    };

    /*
     * ================================================================
     * 🌐 REGISTRA TODAS
     * ================================================================
     */
    for (
        int i = 0;
        i < sizeof(routes) / sizeof(routes[0]);
        i++
    ) {
        httpd_register_uri_handler(
            server,
            &routes[i]
        );
    }

    ESP_LOGI(
        TAG,
        "Rotas API registradas"
    );

    return ESP_OK;
}