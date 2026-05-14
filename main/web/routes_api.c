#include "routes_api.h"

#include "auth.h"

#include "seringa/seringa.h"
#include "motor/motor.h"
#include "calibration/calibration.h"

#include "esp_log.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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
 * Volume padrão usado pelos botões principais da interface.
 *
 * Atualmente:
 *  - INJETAR     -> 1 ml
 *  - RECARREGAR  -> 1 ml
 *
 * Futuramente pode vir da interface web.
 * ============================================================================
 */
#define DEFAULT_ML 1.0f

/*
 * ============================================================================
 * 📊 STATUS -> STRING
 * ============================================================================
 *
 * Converte enum interno da seringa para texto JSON.
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
 *
 * Endpoint:
 *  GET /api/status
 *
 * Retorna:
 *  {
 *      "seringa": "IDLE",
 *      "motor": "IDLE",
 *      "busy": false,
 *      "cheia": false,
 *      "vazia": false
 *  }
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

    /*
     * ================================================================
     * 📦 CONTENT TYPE
     * ================================================================
     */
    httpd_resp_set_type(
        req,
        "application/json"
    );

    /*
     * ================================================================
     * 📊 LEITURA DO DOMÍNIO
     * ================================================================
     */
    seringa_status_t st =
        seringa_get_status();

    /*
     * ================================================================
     * 🧾 JSON
     * ================================================================
     */
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
 *
 * Endpoint:
 *  GET /api/inc
 *
 * Executa:
 *  - injeta DEFAULT_ML
 * ============================================================================
 */
static esp_err_t api_inc_handler(
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

    /*
     * ================================================================
     * 🚫 MOTOR OCUPADO
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
     * 💉 EXECUTA INJEÇÃO
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
 *
 * Endpoint:
 *  GET /api/dec
 *
 * Executa:
 *  - recarrega DEFAULT_ML
 * ============================================================================
 */
static esp_err_t api_dec_handler(
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

    /*
     * ================================================================
     * 🚫 MOTOR OCUPADO
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
     * ♻️ EXECUTA RECARGA
     * ================================================================
     */
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
 *
 * Endpoint:
 *  GET /api/fill
 *
 * Executa:
 *  - retrai até fim de curso traseiro
 * ============================================================================
 */
static esp_err_t api_fill_handler(
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

    /*
     * ================================================================
     * 🚫 MOTOR OCUPADO
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
     * 🧪 EXECUTA ENCHIMENTO
     * ================================================================
     */
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
 * 🧪 GET CALIBRATION
 * ============================================================================
 *
 * Endpoint:
 *  GET /api/calibration/get
 *
 * Retorna:
 *  {
 *      "steps_per_ml": 4000.00
 *  }
 * ============================================================================
 */
static esp_err_t api_calibration_get_handler(
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

    /*
     * ================================================================
     * 📦 CONTENT TYPE
     * ================================================================
     */
    httpd_resp_set_type(
        req,
        "application/json"
    );

    /*
     * ================================================================
     * 📊 VALOR ATUAL
     * ================================================================
     */
    float value =
        calibration_get();

    /*
     * ================================================================
     * 🧾 JSON
     * ================================================================
     */
    char json[128];

    snprintf(
        json,
        sizeof(json),

        "{"
        "\"steps_per_ml\":%.2f"
        "}",

        value
    );

    return httpd_resp_sendstr(
        req,
        json
    );
}

/*
 * ============================================================================
 * 🧪 SET CALIBRATION
 * ============================================================================
 *
 * Endpoint:
 *  POST /api/calibration/set
 *
 * Body:
 *  número em texto
 *
 * Exemplo:
 *  4000
 *
 * Fluxo:
 *  frontend
 *      ↓
 *  routes_api
 *      ↓
 *  calibration_set()
 *      ↓
 *  storage_save_steps_per_ml()
 *      ↓
 *  NVS
 * ============================================================================
 */
static esp_err_t api_calibration_set_handler(
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

    /*
     * ================================================================
     * 📥 LÊ BODY
     * ================================================================
     *
     * O frontend envia o valor como texto simples.
     */
    char buf[64];

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
            "Body inválido"
        );
    }

    buf[len] = '\0';

    /*
     * ================================================================
     * 🔄 STRING -> FLOAT
     * ================================================================
     */
    errno = 0;

    char *endptr = NULL;

    float value =
        strtof(
            buf,
            &endptr
        );

    /*
     * ================================================================
     * ⚠️ VALIDAÇÃO DO TEXTO RECEBIDO
     * ================================================================
     *
     * Regras:
     *  - precisa iniciar com um número válido
     *  - precisa ser positivo
     *  - não pode ter overflow/underflow reportado por strtof()
     *  - depois do número só aceitamos whitespace final
     *
     * IMPORTANTE:
     *  calibration_set() continua sendo a fronteira de segurança fina:
     *  ela aplica clamp nos limites mecânicos permitidos.
     */
    while (
        endptr != NULL &&
        *endptr != '\0' &&
        isspace((unsigned char)*endptr)
    ) {
        endptr++;
    }

    if (
        endptr == buf ||
        errno == ERANGE ||
        value <= 0.0f ||
        *endptr != '\0'
    ) {
        return httpd_resp_send_err(
            req,
            HTTPD_400_BAD_REQUEST,
            "Valor inválido"
        );
    }

    /*
     * ================================================================
     * 💾 APLICA E PERSISTE
     * ================================================================
     */
    calibration_set(value);

    ESP_LOGI(
        TAG,
        "Calibração atualizada: %.2f steps/ml",
        calibration_get()
    );

    return httpd_resp_sendstr(
        req,
        "OK"
    );
}

/*
 * ============================================================================
 * 🛑 STOP
 * ============================================================================
 *
 * Endpoint:
 *  GET /api/stop
 * ============================================================================
 */
static esp_err_t api_stop_handler(
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

    /*
     * ================================================================
     * 🛑 STOP GLOBAL
     * ================================================================
     */
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
 *
 * Centraliza endpoints REST.
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
         * 🧪 GET CALIBRATION
         * ============================================================
         */
        {
            "/api/calibration/get",
            HTTP_GET,
            api_calibration_get_handler,
            NULL
        },

        /*
         * ============================================================
         * 🧪 SET CALIBRATION
         * ============================================================
         */
        {
            "/api/calibration/set",
            HTTP_POST,
            api_calibration_set_handler,
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
     * 🌐 REGISTRA TODAS AS ROTAS
     * ================================================================
     */
    for (
        int i = 0;
        i < sizeof(routes) / sizeof(routes[0]);
        i++
    ) {
        esp_err_t err =
            httpd_register_uri_handler(
                server,
                &routes[i]
            );

        if (err != ESP_OK) {

            ESP_LOGE(
                TAG,
                "Falha registrando rota %s",
                routes[i].uri
            );

            return err;
        }
    }

    ESP_LOGI(
        TAG,
        "Rotas API registradas"
    );

    return ESP_OK;
}
