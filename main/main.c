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
#include "motor/motor_config.h"
#include "seringa/seringa.h"
#include "storage/storage.h"

#include "driver/gpio.h"

#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_random.h"
#include "esp_timer.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdio.h>
#include <stdlib.h>

#define ENDSTOP_STANDALONE_SCAN_COUNT 8
#define ENDSTOP_STANDALONE_DRIVE_GPIO 25

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

#if MOTOR_ENDSTOP_STANDALONE_TEST_ENABLE
/*
 * ============================================================================
 * 🔎 TESTE ISOLADO DOS ENDSTOPS
 * ============================================================================
 *
 * Modo de bancada:
 *  - não inicializa NVS, motor, WiFi, storage nem servidor web
 *  - configura somente GPIO26/GPIO27 como entrada com pulldown
 *  - imprime os níveis crus em loop direto no app_main
 *
 * Objetivo:
 *  separar falha elétrica/pinagem/monitor serial de qualquer lógica do
 *  restante do firmware.
 * ============================================================================
 */
static void endstop_standalone_test(void)
{
    const int scan_gpios[ENDSTOP_STANDALONE_SCAN_COUNT] = {
        13,
        14,
        16,
        17,
        26,
        27,
        32,
        33
    };

    gpio_config_t input_conf = {
        .pin_bit_mask = 0,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    for (int i = 0; i < ENDSTOP_STANDALONE_SCAN_COUNT; i++) {
        input_conf.pin_bit_mask |=
            1ULL << scan_gpios[i];

        ESP_ERROR_CHECK(
            gpio_reset_pin(
                scan_gpios[i]
            )
        );
    }

    ESP_ERROR_CHECK(
        gpio_reset_pin(
            ENDSTOP_STANDALONE_DRIVE_GPIO
        )
    );

    ESP_ERROR_CHECK(
        gpio_config(
            &input_conf
        )
    );

    for (int i = 0; i < ENDSTOP_STANDALONE_SCAN_COUNT; i++) {
        ESP_ERROR_CHECK(
            gpio_set_pull_mode(
                scan_gpios[i],
                GPIO_PULLDOWN_ONLY
            )
        );
    }

    gpio_config_t drive_conf = {
        .pin_bit_mask =
            1ULL << ENDSTOP_STANDALONE_DRIVE_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    ESP_ERROR_CHECK(
        gpio_config(
            &drive_conf
        )
    );

    ESP_ERROR_CHECK(
        gpio_set_level(
            ENDSTOP_STANDALONE_DRIVE_GPIO,
            1
        )
    );

    printf("\n\n");
    printf("========================================\n");
    printf(" ENDSTOP STANDALONE TEST\n");
    printf(" GPIO%d/front/vazio: pulldown, active HIGH\n",
        MOTOR_ENDSTOP_FRONT_GPIO);
    printf(" GPIO%d/back/cheio: pulldown, active HIGH\n",
        MOTOR_ENDSTOP_BACK_GPIO);
    printf(" Scan GPIOs:");
    for (int i = 0; i < ENDSTOP_STANDALONE_SCAN_COUNT; i++) {
        printf(" %d", scan_gpios[i]);
    }
    printf("\n");
    printf(" GPIO%d configurado como SAIDA HIGH para jumper de teste\n",
        ENDSTOP_STANDALONE_DRIVE_GPIO);
    printf(" Firmware normal BLOQUEADO para teste\n");
    printf("========================================\n");
    fflush(stdout);

    while (true) {

        int front_level =
            gpio_get_level(
                MOTOR_ENDSTOP_FRONT_GPIO
            );

        int back_level =
            gpio_get_level(
                MOTOR_ENDSTOP_BACK_GPIO
            );

        printf(
            "ENDSTOP STANDALONE: GPIO%d=%d GPIO%d=%d\n",
            MOTOR_ENDSTOP_FRONT_GPIO,
            front_level,
            MOTOR_ENDSTOP_BACK_GPIO,
            back_level
        );

        printf("GPIO SCAN:");
        printf(
            " %d=OUT_HIGH",
            ENDSTOP_STANDALONE_DRIVE_GPIO
        );
        for (int i = 0; i < ENDSTOP_STANDALONE_SCAN_COUNT; i++) {
            printf(
                " %d=%d",
                scan_gpios[i],
                gpio_get_level(
                    scan_gpios[i]
                )
            );
        }
        printf("\n");
        fflush(stdout);

        vTaskDelay(
            pdMS_TO_TICKS(250)
        );
    }
}
#endif

/*
 * ============================================================================
 * 🚀 ENTRY POINT
 * ============================================================================
 */
void app_main(void)
{
#if MOTOR_ENDSTOP_STANDALONE_TEST_ENABLE
    endstop_standalone_test();
#endif

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
