/*
 * ============================================================================
 * 🧠 MAIN - SISTEMA SERINGA AUTOMATIZADA
 * ============================================================================
 *
 * Responsável por:
 *  - Inicializar NVS
 *  - Inicializar RNG seguro
 *  - Inicializar motor
 *  - Conectar WiFi
 *  - Subir servidor web
 *
 * Arquitetura:
 *  main → inicialização apenas
 *  motor → controle assíncrono (FreeRTOS)
 *  routes → API HTTP
 * ============================================================================
 */

#include "nvs_flash.h"

#include "wifi/wifi.h"
#include "web/web_server.h"

#include "motor/motor.h"

#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdlib.h>

/*
 * TAG de log
 */
static const char *TAG = "MAIN";

/*
 * ============================================================================
 * 🔐 SEED DO RANDOM (IMPORTANTE PARA SEGURANÇA)
 * ============================================================================
 *
 * Usa:
 *  - esp_random() → RNG de hardware
 *  - esp_timer → entropia adicional
 */
static void seed_random(void)
{
    uint32_t seed = esp_random() ^ (uint32_t)esp_timer_get_time();
    srand(seed);

    ESP_LOGI(TAG, "Random seed inicializado: %lu", (unsigned long)seed);
}

/*
 * ============================================================================
 * 💾 INICIALIZA NVS
 * ============================================================================
 */
static void init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {

        ESP_LOGW(TAG, "NVS inconsistente, formatando...");

        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_LOGI(TAG, "NVS OK");
}

/*
 * ============================================================================
 * 🚀 ENTRY POINT
 * ============================================================================
 */
void app_main(void)
{
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "  SISTEMA SERINGA INICIANDO");
    ESP_LOGI(TAG, "=================================");

    /*
     * 1️⃣ NVS
     */
    init_nvs();

    /*
     * 2️⃣ RNG seguro
     */
    seed_random();

    /*
     * 3️⃣ MOTOR (cria task interna)
     */
    ESP_LOGI(TAG, "Inicializando motor...");
    motor_init();

    /*
     * 4️⃣ WIFI
     */
    ESP_LOGI(TAG, "Conectando WiFi...");
    wifi_init();

    /*
     * ⚠️ Ideal: usar evento (futuro)
     * Aqui mantemos delay simples
     */
    vTaskDelay(pdMS_TO_TICKS(2000));

    /*
     * 5️⃣ SERVIDOR WEB
     */
    ESP_LOGI(TAG, "Subindo servidor HTTP...");
    start_webserver();

    /*
     * SISTEMA PRONTO
     */
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "  SISTEMA PRONTO");
    ESP_LOGI(TAG, "=================================");
}