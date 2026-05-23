/*
 * Copyright 2026 clesiorki
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license information.
 */

#include "motor_motion.h"

#include "motor_hw.h"
#include "motor_config.h"

#include "esp_log.h"
#include "esp_rom_sys.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * ============================================================================
 * 🧾 TAG
 * ============================================================================
 */
static const char *TAG = "MOTOR_MOTION";

/*
 * ============================================================================
 * 🔄 ÍNDICE GLOBAL DA SEQUÊNCIA
 * ============================================================================
 *
 * Mantido entre movimentos para:
 *  - reduzir trancos
 *  - evitar perda de sincronismo
 *  - manter continuidade eletromagnética
 * ============================================================================
 */
static int g_step_index = 0;

/*
 * ============================================================================
 * ⏱️ DELAY DINÂMICO (RAMPA)
 * ============================================================================
 *
 * Estratégia:
 *
 *  início:
 *      mais lento
 *
 *  meio:
 *      velocidade máxima
 *
 *  final:
 *      desaceleração
 *
 * Benefícios:
 *  - reduz perda de passo
 *  - reduz vibração
 *  - melhora torque
 *  - reduz solavancos
 * ============================================================================
 */
static inline uint32_t compute_delay_us(
    const motor_motion_profile_t *profile,
    uint32_t current_step
)
{
    /*
     * ================================================================
     * 🚫 RAMPA DESABILITADA
     * ================================================================
     */
    if (!profile->use_ramp) {

        return MOTOR_RAMP_MIN_DELAY_US;
    }

    /*
     * ================================================================
     * 🚫 MOVIMENTO PEQUENO
     * ================================================================
     *
     * Rampas curtas podem piorar
     * movimentos pequenos.
     * ================================================================
     */
    if (profile->steps < (MOTOR_RAMP_STEPS * 2)) {

        return MOTOR_RAMP_START_DELAY_US;
    }

    /*
     * ================================================================
     * 🚀 ACELERAÇÃO
     * ================================================================
     */
    if (current_step < MOTOR_RAMP_STEPS) {

        uint32_t delta =
            MOTOR_RAMP_START_DELAY_US -
            MOTOR_RAMP_MIN_DELAY_US;

        return
            MOTOR_RAMP_START_DELAY_US -
            ((delta * current_step) /
             MOTOR_RAMP_STEPS);
    }

    /*
     * ================================================================
     * 🛑 DESACELERAÇÃO
     * ================================================================
     */
    if (
        current_step >
        (profile->steps - MOTOR_RAMP_STEPS)
    ) {
        uint32_t remain =
            profile->steps -
            current_step;

        uint32_t delta =
            MOTOR_RAMP_START_DELAY_US -
            MOTOR_RAMP_MIN_DELAY_US;

        return
            MOTOR_RAMP_START_DELAY_US -
            ((delta * remain) /
             MOTOR_RAMP_STEPS);
    }

    /*
     * ================================================================
     * ⚡ CRUZEIRO
     * ================================================================
     */
    return MOTOR_RAMP_MIN_DELAY_US;
}

/*
 * ============================================================================
 * 🔄 EXECUTA PASSO
 * ============================================================================
 *
 * IMPORTANTE:
 *  - índice global contínuo
 *  - evita solavancos
 * ============================================================================
 */
static inline void execute_step(
    bool direction
)
{
    motor_hw_apply_step(
        g_step_index
    );

    g_step_index =
        direction
        ? (g_step_index + 1) % 8
        : (g_step_index - 1 + 8) % 8;
}

static inline bool endstop_hit(
    bool direction
);

#if MOTOR_ANTI_STICTION_ENABLE

/*
 * ============================================================================
 * 🔁 COMPENSAÇÃO ANTI-STICTION
 * ============================================================================
 *
 * Objetivo:
 *  - aliviar tensão torsional
 *  - reduzir stick-slip
 *  - reduzir pulsação
 *  - reduzir "solavancos"
 *
 * Estratégia:
 *  - aplica uma micro-retração no sentido oposto ao movimento principal
 *  - retorna a mesma quantidade de passos no sentido nominal
 *
 * IMPORTANTE:
 *  - a compensação é neutra em posição nominal
 *  - o atraso é próprio da compensação, configurado em motor_config.h
 *  - a chamada ocorre apenas quando o perfil do movimento permite
 * ============================================================================
 */
static bool execute_anti_stiction(
    bool direction,
    volatile bool *stop_requested
)
{

    /*
     * ================================================================
     * 🔁 MICRO-RECUO
     * ================================================================
     */
    for (
        uint32_t i = 0;
        i < MOTOR_ANTI_STICTION_BACKSTEPS;
        i++
    ) {
        if (*stop_requested || endstop_hit(direction)) {

            return false;
        }

        execute_step(!direction);

        if (*stop_requested || endstop_hit(direction)) {

            return false;
        }

        esp_rom_delay_us(
            MOTOR_ANTI_STICTION_DELAY_US
        );
    }

    /*
     * ================================================================
     * 🔁 MICRO-AVANÇO
     * ================================================================
     *
     * Recupera posição nominal.
     * ================================================================
     */
    for (
        uint32_t i = 0;
        i < MOTOR_ANTI_STICTION_BACKSTEPS;
        i++
    ) {
        if (*stop_requested || endstop_hit(direction)) {

            return false;
        }

        execute_step(direction);

        if (*stop_requested || endstop_hit(direction)) {

            return false;
        }

        esp_rom_delay_us(
            MOTOR_ANTI_STICTION_DELAY_US
        );
    }

    return true;
}

#endif

/*
 * ============================================================================
 * 🔴 VALIDA ENDSTOP
 * ============================================================================
 */
static inline bool endstop_hit(
    bool direction
)
{
    if (direction) {

        return
            motor_hw_front_endstop_triggered();
    }

    return
        motor_hw_back_endstop_triggered();
}

/*
 * ============================================================================
 * ⚙️ EXECUÇÃO PRINCIPAL
 * ============================================================================
 *
 * IMPORTANTE:
 *  - bloqueante
 *  - roda dentro da motor_task
 *  - alta previsibilidade temporal
 * ============================================================================
 */
void motor_motion_execute(
    const motor_motion_profile_t *profile,
    volatile bool *stop_requested
)
{
    /*
     * ================================================================
     * ⚠️ VALIDAÇÃO
     * ================================================================
     */
    if (profile == NULL) {

        ESP_LOGE(
            TAG,
            "Profile NULL"
        );

        return;
    }

    if (stop_requested == NULL) {

        ESP_LOGE(
            TAG,
            "Stop flag NULL"
        );

        return;
    }

    /*
     * ================================================================
     * 🚫 STEPS INVÁLIDOS
     * ================================================================
     */
    if (profile->steps == 0) {

        ESP_LOGW(
            TAG,
            "Steps = 0"
        );

        return;
    }

    /*
     * ================================================================
     * 🔴 PROTEÇÃO PRÉ-MOVIMENTO
     * ================================================================
     */
    if (endstop_hit(profile->direction)) {

        ESP_LOGW(
            TAG,
            "Endstop já acionado"
        );

        return;
    }

    /*
     * ================================================================
     * 🔄 EXECUÇÃO
     * ================================================================
     */
#if MOTOR_ANTI_STICTION_ENABLE
    uint32_t anti_stiction_counter = 0;
#endif

    for (
        uint32_t step = 0;
        step < profile->steps;
        step++
    ) {

        /*
         * ============================================================
         * 🛑 STOP IMEDIATO
         * ============================================================
         */
        if (*stop_requested) {

            ESP_LOGW(
                TAG,
                "STOP solicitado"
            );

            break;
        }

        /*
         * ============================================================
         * 🔴 ENDSTOP
         * ============================================================
         */
        if (endstop_hit(profile->direction)) {

            ESP_LOGW(
                TAG,
                "Fim de curso atingido"
            );

            break;
        }

        /*
         * ============================================================
         * 🔄 STEP
         * ============================================================
         */
        execute_step(
            profile->direction
        );

        /*
         * ============================================================
         * 🔴 ENDSTOP APÓS PASSO
         * ============================================================
         *
         * O sensor pode mudar exatamente durante a comutação. Conferir
         * novamente antes do delay reduz a parada ao menor intervalo
         * possível sem usar interrupção de GPIO.
         * ============================================================
         */
        if (endstop_hit(profile->direction)) {

            ESP_LOGW(
                TAG,
                "Fim de curso atingido apos passo"
            );

            break;
        }

        /*
         * ============================================================
         * ⏱️ TIMING
         * ============================================================
         */
        uint32_t delay_us =
            compute_delay_us(
                profile,
                step
            );

        esp_rom_delay_us(delay_us);

        /*
         * ============================================================
         * 🛑 STOP/ENDSTOP APÓS DELAY
         * ============================================================
         */
        if (*stop_requested || endstop_hit(profile->direction)) {

            ESP_LOGW(
                TAG,
                "Movimento interrompido apos delay"
            );

            break;
        }

        /*
         * ============================================================
         * 🔁 ANTI-STICTION
         * ============================================================
         */
#if MOTOR_ANTI_STICTION_ENABLE

        if (profile->anti_stiction) {

            anti_stiction_counter++;

            if (
                anti_stiction_counter >=
                MOTOR_ANTI_STICTION_INTERVAL
            ) {
                anti_stiction_counter = 0;

                if (!execute_anti_stiction(
                        profile->direction,
                        stop_requested
                    )) {

                    ESP_LOGW(
                        TAG,
                        "Anti-stiction interrompido"
                    );

                    break;
                }
            }
        }

#endif
    }

    /*
     * ================================================================
     * 🔌 DESENERGIZA BOBINAS
     * ================================================================
     *
     * IMPORTANTE:
     *  reduz aquecimento
     *  reduz consumo
     * =========================================================================
     */
    motor_hw_coils_off();
}
