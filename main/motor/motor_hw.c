/*
 * Copyright 2026 clesiorki
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license information.
 */

#include "motor_hw.h"

#include "motor_config.h"

#include "driver/gpio.h"

#include "esp_log.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * ============================================================================
 * 🧾 TAG
 * ============================================================================
 */
static const char *TAG = "MOTOR_HW";

/*
 * ============================================================================
 * 🔄 SEQUÊNCIA HALF-STEP
 * ============================================================================
 *
 * ULN2003 + 28BYJ-48
 *
 * IMPORTANTE:
 *  - sequência otimizada para torque suave
 *  - reduz vibração
 *  - reduz perda de passo
 * ============================================================================
 */
static const uint8_t g_halfstep_seq[8][4] = {

    {1,0,0,0},
    {1,1,0,0},
    {0,1,0,0},
    {0,1,1,0},
    {0,0,1,0},
    {0,0,1,1},
    {0,0,0,1},
    {1,0,0,1}
};

/*
 * ============================================================================
 * 🔌 ESCREVE BOBINAS
 * ============================================================================
 *
 * IMPORTANTE:
 *  - acesso direto ao hardware
 *  - chamado em tempo real
 * ============================================================================
 */
static inline void set_coils(
    uint8_t a,
    uint8_t b,
    uint8_t c,
    uint8_t d
)
{
    gpio_set_level(
        MOTOR_GPIO_IN1,
        a
    );

    gpio_set_level(
        MOTOR_GPIO_IN2,
        b
    );

    gpio_set_level(
        MOTOR_GPIO_IN3,
        c
    );

    gpio_set_level(
        MOTOR_GPIO_IN4,
        d
    );
}

/*
 * ============================================================================
 * 🔴 CONFIGURA ENDSTOP
 * ============================================================================
 */
#if MOTOR_ENDSTOPS_INSTALLED
static void configure_endstop_gpio(
    int gpio,
    bool internal_pulldown
)
{
    /*
     * Endstops ativos em HIGH precisam de referência LOW em repouso.
     * Nos GPIOs atuais essa referência pode ser feita por pulldown interno.
     */
    gpio_config_t input_conf = {

        .pin_bit_mask = (1ULL << gpio),

        .mode = GPIO_MODE_INPUT,

        .pull_up_en = GPIO_PULLUP_DISABLE,

        .pull_down_en = internal_pulldown
            ? GPIO_PULLDOWN_ENABLE
            : GPIO_PULLDOWN_DISABLE,

        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(
        &input_conf
    );
}
#endif

/*
 * ============================================================================
 * 🚀 INIT
 * ============================================================================
 */
void motor_hw_init(void)
{
    /*
     * ================================================================
     * 🔌 SAÍDAS MOTOR
     * ================================================================
     */
    gpio_config_t output_conf = {

        .pin_bit_mask =

            (1ULL << MOTOR_GPIO_IN1) |
            (1ULL << MOTOR_GPIO_IN2) |
            (1ULL << MOTOR_GPIO_IN3) |
            (1ULL << MOTOR_GPIO_IN4),

        .mode = GPIO_MODE_OUTPUT,

        .pull_up_en = GPIO_PULLUP_DISABLE,

        .pull_down_en = GPIO_PULLDOWN_DISABLE,

        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(
        &output_conf
    );

#if MOTOR_ENDSTOPS_INSTALLED

    /*
     * ================================================================
     * 🔴 ENDSTOPS
     * ================================================================
     *
     * Ativos em HIGH.
     *
     * IMPORTANTE:
     *  - esta configuração só é habilitada quando os sensores físicos
     *    estiverem instalados no hardware
     *  - com sensores ausentes, as leituras públicas retornam false
     *    para evitar estados CHEIA/VAZIA falsos por ruído ou montagem
     *    incompleta
     * ================================================================
     */
    configure_endstop_gpio(
        MOTOR_ENDSTOP_FRONT_GPIO,
        true
    );

    configure_endstop_gpio(
        MOTOR_ENDSTOP_BACK_GPIO,
        true
    );

#else

    ESP_LOGW(
        TAG,
        "Endstops desabilitados por configuracao"
    );

#endif

    /*
     * ================================================================
     * 🔌 GARANTE MOTOR DESLIGADO
     * ================================================================
     */
    motor_hw_coils_off();

    ESP_LOGI(
        TAG,
        "Hardware inicializado"
    );
}

/*
 * ============================================================================
 * 🔄 EXECUTA HALF-STEP
 * ============================================================================
 *
 * step_index:
 *  0-7
 * ============================================================================
 */
void motor_hw_apply_step(
    uint8_t step_index
)
{
    /*
     * ================================================================
     * 🔒 PROTEÇÃO
     * ================================================================
     */
    step_index %= 8;

    /*
     * ================================================================
     * 🔄 APLICA SEQUÊNCIA
     * ================================================================
     */
    set_coils(

        g_halfstep_seq[step_index][0],
        g_halfstep_seq[step_index][1],
        g_halfstep_seq[step_index][2],
        g_halfstep_seq[step_index][3]
    );
}

/*
 * ============================================================================
 * 🔌 DESENERGIZA MOTOR
 * ============================================================================
 *
 * Benefícios:
 *  - reduz aquecimento
 *  - reduz consumo
 *  - aumenta vida útil
 * ============================================================================
 */
void motor_hw_coils_off(void)
{
    set_coils(
        0,
        0,
        0,
        0
    );
}

/*
 * ============================================================================
 * 🔴 LEITURA ENDSTOP FRONTAL
 * ============================================================================
 *
 * Retorna:
 *  true  -> acionado
 *  false -> livre
 *
 * IMPORTANTE:
 *  - ativo conforme MOTOR_ENDSTOP_ACTIVE_LEVEL
 * ============================================================================
 */
bool motor_hw_front_endstop_triggered(void)
{
#if !MOTOR_ENDSTOPS_INSTALLED
    return false;
#endif

    return
        gpio_get_level(
            MOTOR_ENDSTOP_FRONT_GPIO
        ) == MOTOR_ENDSTOP_ACTIVE_LEVEL;
}

/*
 * ============================================================================
 * 🔴 LEITURA ENDSTOP TRASEIRO
 * ============================================================================
 *
 * Retorna:
 *  true  -> acionado
 *  false -> livre
 *
 * IMPORTANTE:
 *  - ativo conforme MOTOR_ENDSTOP_ACTIVE_LEVEL
 * ============================================================================
 */
bool motor_hw_back_endstop_triggered(void)
{
#if !MOTOR_ENDSTOPS_INSTALLED
    return false;
#endif

    return
        gpio_get_level(
            MOTOR_ENDSTOP_BACK_GPIO
        ) == MOTOR_ENDSTOP_ACTIVE_LEVEL;
}
