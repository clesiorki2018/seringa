#include "seringa.h"

#include "seringa_motion.h"

#include "motor.h"

#include "esp_log.h"

/*
 * ============================================================================
 * 🧾 TAG
 * ============================================================================
 */
static const char *TAG = "SERINGA";

/*
 * ============================================================================
 * 🧠 ESTADO INTERNO
 * ============================================================================
 */
static seringa_status_t g_current_status =
    SERINGA_IDLE;

/*
 * ============================================================================
 * 🧠 ATUALIZA STATUS
 * ============================================================================
 *
 * Fonte única da verdade lógica.
 * ============================================================================
 */
static void seringa_update_status(void)
{
    /*
     * ================================================================
     * 🚀 MOVIMENTO TEM PRIORIDADE
     * ================================================================
     */
    if (motor_is_running()) {

        g_current_status =
            SERINGA_MOVING;

        return;
    }

    /*
     * ================================================================
     * 🔴 ENDSTOP TRASEIRO
     * ================================================================
     */
    if (motor_back_endstop_triggered()) {

        g_current_status =
            SERINGA_CHEIA;

        return;
    }

    /*
     * ================================================================
     * 🔴 ENDSTOP FRONTAL
     * ================================================================
     */
    if (motor_front_endstop_triggered()) {

        g_current_status =
            SERINGA_VAZIA;

        return;
    }

    /*
     * ================================================================
     * 😴 IDLE INTERMEDIÁRIO
     * ================================================================
     */
    g_current_status =
        SERINGA_IDLE;
}

/*
 * ============================================================================
 * 🚀 INIT
 * ============================================================================
 */
void seringa_init(void)
{
    seringa_update_status();

    ESP_LOGI(TAG, "Seringa inicializada");
}

/*
 * ============================================================================
 * 💉 INJEÇÃO
 * ============================================================================
 */
bool seringa_injetar_ml(
    float ml,
    seringa_flow_profile_t profile
)
{
    seringa_update_status();

    /*
     * ================================================================
     * 🚫 MOTOR OCUPADO
     * ================================================================
     */
    if (motor_is_running()) {

        ESP_LOGW(
            TAG,
            "Motor ocupado"
        );

        return false;
    }

    /*
     * ================================================================
     * 🔴 SERINGA VAZIA
     * ================================================================
     */
    if (motor_front_endstop_triggered()) {

        ESP_LOGW(
            TAG,
            "Seringa vazia"
        );

        return false;
    }

    /*
     * ================================================================
     * 🚀 EXECUTA MOVIMENTO
     * ================================================================
     */
    bool success =
        seringa_motion_injetar(
            ml,
            profile
        );

    seringa_update_status();

    return success;
}

/*
 * ============================================================================
 * ♻️ RECARGA
 * ============================================================================
 */
bool seringa_recarregar_ml(
    float ml,
    seringa_flow_profile_t profile
)
{
    seringa_update_status();

    if (motor_is_running()) {

        ESP_LOGW(
            TAG,
            "Motor ocupado"
        );

        return false;
    }

    /*
     * ================================================================
     * 🔴 JÁ ESTÁ CHEIA
     * ================================================================
     */
    if (motor_back_endstop_triggered()) {

        ESP_LOGW(
            TAG,
            "Seringa já cheia"
        );

        return false;
    }

    bool success =
        seringa_motion_recarregar(
            ml,
            profile
        );

    seringa_update_status();

    return success;
}

/*
 * ============================================================================
 * 🧪 ENCHIMENTO TOTAL
 * ============================================================================
 */
bool seringa_encher_total(void)
{
    seringa_update_status();

    if (motor_is_running()) {

        ESP_LOGW(
            TAG,
            "Motor ocupado"
        );

        return false;
    }

    if (motor_back_endstop_triggered()) {

        ESP_LOGW(
            TAG,
            "Já está cheia"
        );

        return false;
    }

    bool success =
        seringa_motion_encher_total();

    seringa_update_status();

    return success;
}

/*
 * ============================================================================
 * 🛑 STOP
 * ============================================================================
 */
void seringa_stop(void)
{
    ESP_LOGW(TAG, "STOP");

    motor_stop();

    seringa_update_status();
}

/*
 * ============================================================================
 * 📊 STATUS
 * ============================================================================
 */
seringa_status_t seringa_get_status(void)
{
    seringa_update_status();

    return g_current_status;
}

/*
 * ============================================================================
 * 🔎 HELPERS
 * ============================================================================
 */

bool seringa_is_busy(void)
{
    return motor_is_running();
}

bool seringa_is_cheia(void)
{
    return motor_back_endstop_triggered();
}

bool seringa_is_vazia(void)
{
    return motor_front_endstop_triggered();
}