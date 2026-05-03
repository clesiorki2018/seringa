#include "web_server.h"
#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "esp_log.h"

static const char *TAG = "WEB";

/*
 * 🔗 Função externa (routes.c)
 */
extern esp_err_t register_routes(httpd_handle_t server);

/*
 * ============================================================================
 * 📂 SPIFFS
 * ============================================================================
 */
static esp_err_t spiffs_init(void)
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 10,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao montar SPIFFS (%d)", ret);
        return ret;
    }

    size_t total = 0, used = 0;

    if (esp_spiffs_info(NULL, &total, &used) == ESP_OK)
    {
        ESP_LOGI(TAG, "SPIFFS OK: total=%d usado=%d", total, used);
    }
    else
    {
        ESP_LOGW(TAG, "Falha ao obter info do SPIFFS");
    }

    return ESP_OK;
}

/*
 * ============================================================================
 * 🌐 SERVIDOR HTTP
 * ============================================================================
 */
void start_webserver(void)
{
    /*
     * 📂 1. SPIFFS
     */
    if (spiffs_init() != ESP_OK)
    {
        ESP_LOGE(TAG, "SPIFFS falhou. Abortando.");
        return;
    }

    /*
     * 🌐 2. Configuração HTTP
     */
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    /*
     * 🔧 Ajustes importantes
     */
    config.max_uri_handlers = 16;   // 🔥 CORRIGIDO (era 8)
    config.stack_size = 6144;       // mais seguro com várias rotas + auth
    config.uri_match_fn = httpd_uri_match_wildcard; // permite expansão futura

    httpd_handle_t server = NULL;

    /*
     * 🚀 3. Start
     */
    esp_err_t ret = httpd_start(&server, &config);

    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Erro ao iniciar HTTP (%d)", ret);
        return;
    }

    /*
     * 🔗 4. Rotas
     */
    register_routes(server);

    ESP_LOGI(TAG, "Servidor HTTP iniciado (completo + seguro)");
}