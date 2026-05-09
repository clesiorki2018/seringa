#include "web_server.h"

#include "routes_api.h"
#include "routes_static.h"

#include "esp_log.h"

/*
 * ============================================================================
 * 🧾 TAG
 * ============================================================================
 */
static const char *TAG = "ROUTES";

/*
 * ============================================================================
 * 🧠 REGISTRO GLOBAL DE ROTAS
 * ============================================================================
 *
 * Responsável apenas por:
 *
 *  - organizar módulos de rota
 *  - registrar APIs
 *  - registrar arquivos estáticos
 *
 * IMPORTANTE:
 *  - NÃO implementar handlers aqui
 *  - NÃO colocar lógica HTTP aqui
 *  - NÃO colocar autenticação aqui
 *
 * Arquitetura:
 *
 *   routes.c
 *      ├── routes_api.c
 *      └── routes_static.c
 *
 * Benefícios:
 *
 *  - manutenção simplificada
 *  - arquivos menores
 *  - menor acoplamento
 *  - menor risco de conflitos
 *  - escalabilidade futura
 * ============================================================================
 */
esp_err_t register_routes(
    httpd_handle_t server
)
{
    /*
     * ================================================================
     * 🌐 ROTAS ESTÁTICAS
     * ================================================================
     */
    ESP_ERROR_CHECK(
        register_static_routes(server)
    );

    /*
     * ================================================================
     * 🔌 API REST
     * ================================================================
     */
    ESP_ERROR_CHECK(
        register_api_routes(server)
    );

    ESP_LOGI(
        TAG,
        "Todas as rotas registradas"
    );

    return ESP_OK;
}