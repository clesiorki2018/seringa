#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "motor/motor.h"
#include "seringa/seringa.h"

/*
 * ============================================================================
 * 🧾 TAG DE LOG
 * ============================================================================
 */
static const char *TAG = "ROUTES";

/*
 * ============================================================================
 * 🔐 CONFIGURAÇÃO DE SEGURANÇA
 * ============================================================================
 *
 * PIN_HASH:
 *  - hash do PIN correto
 *
 * TOKEN:
 *  - sessão baseada em token simples
 *  - expiração automática
 */
#define PIN_HASH  0x7c78c9af

#define TOKEN_SIZE 32
#define SESSION_TIMEOUT_US (15 * 60 * 1000000ULL)

/*
 * ============================================================================
 * 🧠 ESTADO DA SESSÃO
 * ============================================================================
 */
static char session_token[TOKEN_SIZE] = {0};
static int64_t session_expiry = 0;

/*
 * ============================================================================
 * 🔐 HASH SIMPLES (djb2)
 * ============================================================================
 *
 * Rápido, leve, suficiente para PIN local
 */
static uint32_t simple_hash(const char *str)
{
    uint32_t hash = 5381;

    while (*str) {
        hash = ((hash << 5) + hash) + (uint8_t)(*str);
        str++;
    }

    return hash;
}

/*
 * ============================================================================
 * 🔐 GERA TOKEN DE SESSÃO
 * ============================================================================
 */
static void generate_token(void)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyz0123456789";

    for (int i = 0; i < TOKEN_SIZE - 1; i++) {
        session_token[i] = charset[rand() % (sizeof(charset) - 1)];
    }

    session_token[TOKEN_SIZE - 1] = '\0';
}

/*
 * ============================================================================
 * 🔐 VALIDA AUTORIZAÇÃO
 * ============================================================================
 *
 * Fluxo:
 * 1. Lê header Authorization
 * 2. Compara token
 * 3. Verifica timeout
 * 4. Renova sessão automaticamente
 */
static bool is_authorized(httpd_req_t *req)
{
    char buf[64];

    if (httpd_req_get_hdr_value_str(req, "Authorization", buf, sizeof(buf)) != ESP_OK)
        return false;

    if (strcmp(buf, session_token) != 0)
        return false;

    int64_t now = esp_timer_get_time();

    if (now > session_expiry) {
        ESP_LOGW(TAG, "Sessão expirada");
        session_token[0] = '\0';
        return false;
    }

    session_expiry = now + SESSION_TIMEOUT_US;

    return true;
}

/*
 * ============================================================================
 * 📂 SERVIDOR DE ARQUIVOS (SPIFFS)
 * ============================================================================
 */
static esp_err_t serve_file(httpd_req_t *req,
                            const char *filepath,
                            const char *content_type)
{
    FILE *file = fopen(filepath, "r");

    if (!file)
        return httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Arquivo não encontrado");

    httpd_resp_set_type(req, content_type);

    char buffer[512];
    size_t read_bytes;

    while ((read_bytes = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (httpd_resp_send_chunk(req, buffer, read_bytes) != ESP_OK) {
            fclose(file);
            return ESP_FAIL;
        }
    }

    fclose(file);
    httpd_resp_send_chunk(req, NULL, 0);

    return ESP_OK;
}

/*
 * ============================================================================
 * 🔐 LOGIN
 * ============================================================================
 */
static esp_err_t login_handler(httpd_req_t *req)
{
    char buf[32];

    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);

    if (len <= 0)
        return httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Erro leitura");

    buf[len] = '\0';

    uint32_t hash = simple_hash(buf);

    ESP_LOGI(TAG, "Tentativa login hash=0x%08lx", (unsigned long)hash);

    if (hash != PIN_HASH)
        return httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "PIN incorreto");

    generate_token();
    session_expiry = esp_timer_get_time() + SESSION_TIMEOUT_US;

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, session_token);

    return ESP_OK;
}

/*
 * ============================================================================
 * 📊 STATUS → JSON
 * ============================================================================
 */
static const char* status_to_str(seringa_status_t st)
{
    switch (st) {
        case SERINGA_CHEIA:   return "CHEIA";
        case SERINGA_VAZIA:   return "VAZIA";
        case SERINGA_MOVING:  return "MOVING";
        default:              return "IDLE";
    }
}

static esp_err_t api_status_handler(httpd_req_t *req)
{
    if (!is_authorized(req))
        return httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Sessão inválida");

    httpd_resp_set_type(req, "application/json");

    seringa_status_t st = seringa_get_status();

    char json[128];

    snprintf(json, sizeof(json),
        "{"
        "\"seringa\":\"%s\","
        "\"motor\":\"%s\","
        "\"busy\":%s"
        "}",
        status_to_str(st),
        motor_is_running() ? "RUNNING" : "IDLE",
        seringa_is_busy() ? "true" : "false"
    );

    return httpd_resp_sendstr(req, json);
}

/*
 * ============================================================================
 * ⚙️ COMANDOS SERINGA
 * ============================================================================
 */

#define DEFAULT_STEPS 2048*7

static esp_err_t api_inc_handler(httpd_req_t *req)
{
    if (!is_authorized(req))
        return httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Sessão inválida");

    if (seringa_is_busy())
        return httpd_resp_sendstr(req, "BUSY");

    seringa_injetar_steps(DEFAULT_STEPS);

    return httpd_resp_sendstr(req, "OK");
}

static esp_err_t api_dec_handler(httpd_req_t *req)
{
    if (!is_authorized(req))
        return httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Sessão inválida");

    if (seringa_is_busy())
        return httpd_resp_sendstr(req, "BUSY");

    seringa_recarregar_steps(DEFAULT_STEPS);

    return httpd_resp_sendstr(req, "OK");
}

static esp_err_t api_stop_handler(httpd_req_t *req)
{
    if (!is_authorized(req))
        return httpd_resp_send_err(req, HTTPD_401_UNAUTHORIZED, "Sessão inválida");

    seringa_stop();

    return httpd_resp_sendstr(req, "OK");
}

/*
 * ============================================================================
 * 🌐 ROTAS WEB
 * ============================================================================
 */
static esp_err_t index_handler(httpd_req_t *req)
{
    return serve_file(req, "/spiffs/index.html", "text/html");
}

static esp_err_t calibragem_handler(httpd_req_t *req)
{
    return serve_file(req, "/spiffs/calibragem.html", "text/html");
}

static esp_err_t js_handler(httpd_req_t *req)
{
    return serve_file(req, "/spiffs/app.js", "application/javascript");
}

static esp_err_t css_handler(httpd_req_t *req)
{
    return serve_file(req, "/spiffs/style.css", "text/css");
}

/*
 * ============================================================================
 * 🧠 REGISTRO DE ROTAS
 * ============================================================================
 */
esp_err_t register_routes(httpd_handle_t server)
{
    httpd_uri_t routes[] = {

        {"/", HTTP_GET, index_handler, NULL},
        {"/calibragem", HTTP_GET, calibragem_handler, NULL},

        {"/app.js", HTTP_GET, js_handler, NULL},
        {"/style.css", HTTP_GET, css_handler, NULL},

        {"/api/login", HTTP_POST, login_handler, NULL},

        {"/api/inc", HTTP_GET, api_inc_handler, NULL},
        {"/api/dec", HTTP_GET, api_dec_handler, NULL},
        {"/api/stop", HTTP_GET, api_stop_handler, NULL},
        {"/api/status", HTTP_GET, api_status_handler, NULL},
    };

    for (int i = 0; i < sizeof(routes) / sizeof(routes[0]); i++) {
        httpd_register_uri_handler(server, &routes[i]);
    }

    ESP_LOGI(TAG, "Rotas registradas com sucesso");

    return ESP_OK;
}