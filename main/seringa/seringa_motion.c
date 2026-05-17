/*
 * Copyright 2026 clesiorki
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license information.
 */

#include "seringa_motion.h"

#include "seringa_config.h"
#include "motor.h"
#include "calibration.h"

#include "esp_log.h"

#include <stdbool.h>
#include <stdint.h>

/*
 * ============================================================================
 * 🧾 TAG
 * ============================================================================
 */
static const char *TAG = "SERINGA_MOTION";

/*
 * ============================================================================
 * 🧠 CONVERSÃO ML -> STEPS
 * ============================================================================
 *
 * Mantém a validação centralizada para evitar duplicação.
 * ============================================================================
 */
static bool seringa_motion_ml_to_steps(
    float ml,
    uint32_t *steps_out
)
{
    if (steps_out == NULL) {
        return false;
    }

    if (ml <= 0.0f) {
        ESP_LOGW(TAG, "Volume inválido: %.3f ml", ml);
        return false;
    }

    uint32_t steps =
        (uint32_t) calibration_ml_to_steps(ml);

    if (steps == 0) {
        ESP_LOGW(TAG, "Conversão resultou em 0 steps");
        return false;
    }

    *steps_out = steps;

    return true;
}

/*
 * ============================================================================
 * 🧠 APLICA PERFIL HIDRÁULICO
 * ============================================================================
 *
 * Traduz o perfil da seringa para um perfil físico do motor.
 *
 * IMPORTANTE:
 *  - a seringa decide o comportamento hidráulico
 *  - o motor apenas executa movimento
 * ============================================================================
 */
static void seringa_motion_apply_profile(
    motor_move_t *move,
    seringa_flow_profile_t profile
)
{
    if (move == NULL) {
        return;
    }

    switch (profile) {

        case SERINGA_FLOW_PRECISION:
            move->use_ramp =
                SERINGA_PRECISION_RAMP;

            move->anti_stiction_enable =
                SERINGA_PRECISION_ANTI_STICTION;
            break;

        case SERINGA_FLOW_SMOOTH:
            move->use_ramp =
                SERINGA_SMOOTH_RAMP;

            move->anti_stiction_enable =
                SERINGA_SMOOTH_ANTI_STICTION;
            break;

        case SERINGA_FLOW_FAST:
            move->use_ramp =
                SERINGA_FAST_RAMP;

            move->anti_stiction_enable =
                SERINGA_FAST_ANTI_STICTION;
            break;

        case SERINGA_FLOW_NORMAL:
        default:
            move->use_ramp =
                SERINGA_NORMAL_RAMP;

            move->anti_stiction_enable =
                SERINGA_NORMAL_ANTI_STICTION;
            break;
    }
}

/*
 * ============================================================================
 * 🚀 EXECUTA MOVIMENTO DA SERINGA
 * ============================================================================
 *
 * Função utilitária para evitar duplicação entre:
 *  - injeção
 *  - recarga
 *  - enchimento total
 * ============================================================================
 */
static bool seringa_motion_execute_steps(
    motor_direction_t direction,
    uint32_t steps,
    seringa_flow_profile_t profile,
    const char *label
)
{
    if (steps == 0) {
        ESP_LOGW(TAG, "Movimento inválido: 0 steps");
        return false;
    }

    motor_move_t move = {

        .direction =
            direction,

        .steps =
            steps,

        .use_ramp =
            true,

        .anti_stiction_enable =
            false
    };

    seringa_motion_apply_profile(
        &move,
        profile
    );

    ESP_LOGI(
        TAG,
        "%s (%lu steps)",
        label,
        (unsigned long) steps
    );

    return motor_move(&move);
}

/*
 * ============================================================================
 * 💉 INJEÇÃO
 * ============================================================================
 */
bool seringa_motion_injetar(
    float ml,
    seringa_flow_profile_t profile
)
{
    uint32_t steps = 0;

    if (!seringa_motion_ml_to_steps(ml, &steps)) {
        return false;
    }

    ESP_LOGI(
        TAG,
        "Injetando %.3f ml",
        ml
    );

    return seringa_motion_execute_steps(
        MOTOR_DIRECTION_FORWARD,
        steps,
        profile,
        "Injeção iniciada"
    );
}

/*
 * ============================================================================
 * ♻️ RECARGA PARCIAL
 * ============================================================================
 */
bool seringa_motion_recarregar(
    float ml,
    seringa_flow_profile_t profile
)
{
    uint32_t steps = 0;

    if (!seringa_motion_ml_to_steps(ml, &steps)) {
        return false;
    }

    ESP_LOGI(
        TAG,
        "Recarregando %.3f ml",
        ml
    );

    return seringa_motion_execute_steps(
        MOTOR_DIRECTION_BACKWARD,
        steps,
        profile,
        "Recarga iniciada"
    );
}

/*
 * ============================================================================
 * 🧪 ENCHIMENTO TOTAL
 * ============================================================================
 *
 * Estratégia:
 *  - envia um número grande de passos
 *  - o motor para ao atingir o endstop traseiro
 *
 * IMPORTANTE:
 *  - o limite físico real é o endstop
 *  - o número de passos é apenas uma margem operacional
 * ============================================================================
 */
bool seringa_motion_encher_total(void)
{
    return seringa_motion_execute_steps(
        MOTOR_DIRECTION_BACKWARD,
        30000,
        SERINGA_FLOW_NORMAL,
        "Enchimento total iniciado"
    );
}