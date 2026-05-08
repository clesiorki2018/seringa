#include "web_server.h"

#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "esp_log.h"

/*
 * ============================================================================
 * 🧾 TAG
 * ============================================================================
 */
static const char *TAG = "WEB";

/*
 * ============================================================================
 * 🔗 DEPENDÊNCIA EXTERNA (ROTEAMENTO)
 * ============================================================================
 *
 * OBS:
 *  - Mantemos desacoplado
 *  - web_server NÃO sabe detalhes das rotas
 *
 * Clean Architecture:
 *  web_server → apenas infraestrutura HTTP
 *  routes     → camada de interface (API)
 */
extern esp_err_t register_routes(httpd_handle_t server);

/*
 * ============================================================================
 * 📂 SPIFFS (FILESYSTEM EMBARCADO)
 * ============================================================================
 *
 * RESPONSABILIDADE:
 *  - montar filesystem
 *  - validar funcionamento
 *
 * IMPORTANTE:
 *  - falha aqui = sistema web inutilizável
 * ============================================================================
 */
static esp_err_t spiffs_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 10,
        .format_if_mount_failed = true  // 🔥 evita brick em produção
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao montar SPIFFS (%d)", ret);
        return ret;
    }

    /*
     * 📊 Diagnóstico de uso
     */
    size_t total = 0, used = 0;

    if (esp_spiffs_info(NULL, &total, &used) == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS OK: total=%d bytes | usado=%d bytes", total, used);
    } else {
        ESP_LOGW(TAG, "Falha ao obter info do SPIFFS");
    }

    return ESP_OK;
}

/*
 * ============================================================================
 * 🌐 CONFIGURAÇÃO HTTP
 * ============================================================================
 *
 * Separado para:
 *  - legibilidade
 *  - manutenção futura
 */
static void configure_httpd(httpd_config_t *config)
{
        httpd_config_t cfg = HTTPD_DEFAULT_CONFIG(); // ✅ correto
        *config = cfg;

    /*
     * 🔧 Ajustes importantes para sistema real
     */
    config->max_uri_handlers = 16;      // suporta crescimento da API
    config->stack_size = 6144;          // evita stack overflow com JSON/Auth
    config->uri_match_fn = httpd_uri_match_wildcard;

    /*
     * 🔐 Hardening básico (opcional futuro)
     *
     * config->lru_purge_enable = true;
     * config->recv_wait_timeout = 5;
     * config->send_wait_timeout = 5;
     */
}

/*
 * ============================================================================
 * 🚀 START DO SERVIDOR
 * ============================================================================
 *
 * FLUXO:
 *  1. Monta SPIFFS
 *  2. Configura HTTP
 *  3. Inicia servidor
 *  4. Registra rotas
 *
 * ERRO:
 *  - qualquer falha interrompe inicialização
 */
void start_webserver(void)
{
    /*
     * 📂 1. Filesystem
     */
    if (spiffs_init() != ESP_OK) {
        ESP_LOGE(TAG, "SPIFFS falhou → servidor abortado");
        return;
    }

    /*
     * 🌐 2. Configuração HTTP
     */
    httpd_config_t config;
    configure_httpd(&config);

    httpd_handle_t server = NULL;

    /*
     * 🚀 3. Start HTTP
     */
    esp_err_t ret = httpd_start(&server, &config);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao iniciar HTTP (%d)", ret);
        return;
    }

    /*
     * 🔗 4. Registro de rotas
     */
    if (register_routes(server) != ESP_OK) {
        ESP_LOGE(TAG, "Falha ao registrar rotas");
        httpd_stop(server);
        return;
    }

    /*
     * ✅ Sistema pronto
     */
    ESP_LOGI(TAG, "Servidor HTTP ativo (SPIFFS + API + Auth)");
}