#include "seringa_motion.h"

#include "seringa_config.h"

#include "motor.h"
#include "calibration.h"

#include "esp_log.h"

#include <stdint.h>

/*
 * ============================================================================
 * 🧾 TAG
 * ============================================================================
 */
static const char *TAG = "SERINGA_MOTION";

/*
 * ============================================================================
 * 🧠 CONFIGURA PERFIL DO MOTOR
 * ============================================================================
 */
static void seringa_motion_apply_profile(
    motor_move_t *move,
    seringa_flow_profile_t profile
)
{
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
 * 💉 INJEÇÃO
 * ============================================================================
 */
bool seringa_motion_injetar(
    float ml,
    seringa_flow_profile_t profile
)
{
    /*
     * ================================================================
     * ⚠️ VALIDAÇÃO
     * ================================================================
     */
    if (ml <= 0.0f) {

        ESP_LOGW(TAG, "ML inválido");

        return false;
    }

    /*
     * ================================================================
     * 🔄 CONVERSÃO ML -> STEPS
     * ================================================================
     */
    uint32_t steps =
        calibration_ml_to_steps(ml);

    if (steps == 0) {

        ESP_LOGW(TAG, "Conversão resultou em 0 steps");

        return false;
    }

    /*
     * ================================================================
     * ⚙️ PERFIL DO MOTOR
     * ================================================================
     */
    motor_move_t move = {

        .direction =
            MOTOR_DIRECTION_FORWARD,

        .steps = steps,

        .use_ramp = true,

        .anti_stiction_enable = true
    };

    seringa_motion_apply_profile(
        &move,
        profile
    );

    /*
     * ================================================================
     * 🚀 EXECUTA
     * ================================================================
     */
    ESP_LOGI(
        TAG,
        "Injetando %.2f ml (%lu steps)",
        ml,
        (unsigned long)steps
    );

    return motor_move(&move);
}

/*
 * ============================================================================
 * ♻️ RECARGA
 * ============================================================================
 */
bool seringa_motion_recarregar(
    float ml,
    seringa_flow_profile_t profile
)
{
    if (ml <= 0.0f) {
        return false;
    }

    uint32_t steps =
        calibration_ml_to_steps(ml);

    if (steps == 0) {
        return false;
    }

    motor_move_t move = {

        .direction =
            MOTOR_DIRECTION_BACKWARD,

        .steps = steps,

        .use_ramp = true,

        .anti_stiction_enable = false
    };

    seringa_motion_apply_profile(
        &move,
        profile
    );

    ESP_LOGI(
        TAG,
        "Recarregando %.2f ml (%lu steps)",
        ml,
        (unsigned long)steps
    );

    return motor_move(&move);
}

/*
 * ============================================================================
 * 🧪 ENCHIMENTO TOTAL
 * ============================================================================
 *
 * Estratégia:
 *  - envia passos "infinitos"
 *  - motor para no endstop
 * ============================================================================
 */
bool seringa_motion_encher_total(void)
{
    motor_move_t move = {

        .direction =
            MOTOR_DIRECTION_BACKWARD,

        .steps = 30000,

        .use_ramp = true,

        .anti_stiction_enable = false
    };

    ESP_LOGI(TAG, "Enchimento total iniciado");

    return motor_move(&move);
}