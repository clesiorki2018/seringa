/*
 * Copyright 2026 clesiorki
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license information.
 */

/*
 * ============================================================================
 * 🧠 MAIN - SISTEMA SERINGA AUTOMATIZADA
 * ============================================================================
 *
 * Responsável por:
 *  - Inicializar NVS
 *  - Inicializar RNG seguro
 *  - Inicializar storage lógico
 *  - Carregar calibração persistida
 *  - Inicializar motor
 *  - Sincronizar domínio da seringa
 *  - Conectar WiFi
 *  - Subir servidor web
 *
 * Arquitetura:
 *  main → inicialização apenas
 *  motor → controle assíncrono (FreeRTOS)
 *  seringa → domínio lógico
 *  routes → API HTTP
 * ============================================================================
 */

#include "nvs_flash.h"

#include "calibration/calibration.h"
#include "wifi/wifi.h"
#include "web/web_server.h"

#include "motor/motor.h"
#include "seringa/seringa.h"
#include "storage/storage.h"

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
 * 🧠 INICIALIZA CAMADA DE APLICAÇÃO
 * ============================================================================
 *
 * Ordem intencional:
 *
 *  1. storage
 *      - valida que o namespace lógico está disponível sobre a NVS já aberta
 *
 *  2. calibration
 *      - carrega steps/ml persistido
 *      - aplica fallback seguro se ainda não houver valor salvo
 *
 *  3. seringa
 *      - sincroniza o estado lógico com motor/endstops
 *
 * IMPORTANTE:
 *  - esta função não configura GPIO nem WiFi
 *  - cada módulo mantém sua própria responsabilidade
 *  - main apenas define a sequência de boot do sistema
 * ============================================================================
 */
static void init_application_state(void)
{
    ESP_LOGI(TAG, "Inicializando storage...");
    storage_init();

    ESP_LOGI(TAG, "Carregando calibração...");
    calibration_init();

    ESP_LOGI(TAG, "Sincronizando domínio da seringa...");
    seringa_init();
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
     * 3️⃣ MOTOR (cria task interna e inicializa GPIO/endstops)
     */
    ESP_LOGI(TAG, "Inicializando motor...");
    motor_init();

    /*
     * 4️⃣ ESTADO DA APLICAÇÃO
     *
     * Deve ocorrer depois do motor porque seringa_init()
     * consulta endstops através da fachada pública do motor.
     */
    init_application_state();

    /*
     * 5️⃣ WIFI
     */
    ESP_LOGI(TAG, "Conectando WiFi...");
    wifi_init();

    /*
     * ⚠️ Ideal: usar evento (futuro)
     * Aqui mantemos delay simples
     */
    vTaskDelay(pdMS_TO_TICKS(2000));

    /*
     * 7️⃣ SERVIDOR WEB
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
