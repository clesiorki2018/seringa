#include "motor_motion.h"

#include "motor_hw.h"
#include "motor_config.h"

#include "esp_rom_sys.h"

/*
 * ============================================================================
 * 🧠 DELAY DINÂMICO DA RAMPA
 * ============================================================================
 *
 * Gera rampa trapezoidal simples:
 *
 *  lento -> rápido -> lento
 *
 * Benefícios:
 *  - reduz trancos
 *  - reduz torção do acoplador
 *  - reduz perda de passo
 *  - melhora suavidade
 * ============================================================================
 */
static uint32_t motor_motion_compute_delay(
    uint32_t current_step,
    uint32_t total_steps
)
{
    /*
     * Sem rampa possível.
     */
    if (total_steps < (MOTOR_RAMP_STEPS * 2)) {
        return MOTOR_RAMP_START_DELAY_US;
    }

    /*
     * =========================================================================
     * 🚀 RAMPA DE ACELERAÇÃO
     * =========================================================================
     */
    if (current_step < MOTOR_RAMP_STEPS) {

        uint32_t delta =
            MOTOR_RAMP_START_DELAY_US -
            MOTOR_RAMP_MIN_DELAY_US;

        return MOTOR_RAMP_START_DELAY_US -
            ((delta * current_step) / MOTOR_RAMP_STEPS);
    }

    /*
     * =========================================================================
     * 🛑 RAMPA DE DESACELERAÇÃO
     * =========================================================================
     */
    uint32_t remaining_steps = total_steps - current_step;

    if (remaining_steps < MOTOR_RAMP_STEPS) {

        uint32_t delta =
            MOTOR_RAMP_START_DELAY_US -
            MOTOR_RAMP_MIN_DELAY_US;

        return MOTOR_RAMP_MIN_DELAY_US +
            ((delta * (MOTOR_RAMP_STEPS - remaining_steps))
             / MOTOR_RAMP_STEPS);
    }

    /*
     * =========================================================================
     * ⚡ VELOCIDADE CRUZEIRO
     * =========================================================================
     */
    return MOTOR_RAMP_MIN_DELAY_US;
}

/*
 * ============================================================================
 * 🔄 EXECUTA UM SEGMENTO
 * ============================================================================
 *
 * Movimento dividido em blocos menores.
 *
 * Benefícios:
 *  - STOP mais responsivo
 *  - compensação mecânica
 *  - menor erro acumulado
 * ============================================================================
 */
static bool motor_motion_execute_segment(
    bool direction,
    uint32_t segment_steps,
    uint32_t total_steps,
    uint32_t base_step,
    bool use_ramp,
    volatile bool *stop_requested
)
{
    for (uint32_t i = 0; i < segment_steps; i++) {

        /*
         * ================================================================
         * 🛑 STOP IMEDIATO
         * ================================================================
         */
        if (*stop_requested) {
            return false;
        }

        /*
         * ================================================================
         * 🔴 ENDSTOP
         * ================================================================
         */
        if (direction) {

            if (motor_hw_front_endstop_triggered()) {
                return false;
            }

        } else {

            if (motor_hw_back_endstop_triggered()) {
                return false;
            }
        }

        /*
         * ================================================================
         * 🔄 EXECUTA PASSO
         * ================================================================
         */
        motor_hw_step(direction);

        /*
         * ================================================================
         * ⏱️ DELAY DINÂMICO
         * ================================================================
         */
        uint32_t delay_us;

        if (use_ramp) {

            delay_us = motor_motion_compute_delay(
                base_step + i,
                total_steps
            );

        } else {

            delay_us = MOTOR_RAMP_MIN_DELAY_US;
        }

        esp_rom_delay_us(delay_us);
    }

    return true;
}

/*
 * ============================================================================
 * ⚙️ COMPENSAÇÃO ANTI-STICTION
 * ============================================================================
 *
 * Inspirado em:
 *  - CNC peck drilling
 *  - extrusoras
 *  - bombas industriais
 *
 * Objetivo:
 *  - reduzir stick-slip
 *  - aliviar tensão mecânica
 *  - suavizar fluxo
 * ============================================================================
 */
static void motor_motion_anti_stiction(bool direction)
{
#if MOTOR_ANTI_STICTION_ENABLE

    for (uint32_t i = 0;
         i < MOTOR_ANTI_STICTION_BACKSTEPS;
         i++) {

        motor_hw_step(!direction);

        esp_rom_delay_us(MOTOR_RAMP_START_DELAY_US);
    }

#endif
}

/*
 * ============================================================================
 * 🚀 EXECUÇÃO PRINCIPAL
 * ============================================================================
 */
void motor_motion_execute(
    const motor_motion_profile_t *profile,
    volatile bool *stop_requested
)
{
    uint32_t executed_steps = 0;

    while (executed_steps < profile->steps) {

        /*
         * ================================================================
         * 🛑 STOP GLOBAL
         * ================================================================
         */
        if (*stop_requested) {
            break;
        }

        /*
         * ================================================================
         * 📦 DEFINE TAMANHO DO SEGMENTO
         * ================================================================
         */
        uint32_t remaining_steps =
            profile->steps - executed_steps;

        uint32_t segment_steps =
            remaining_steps > MOTOR_SEGMENT_STEPS
                ? MOTOR_SEGMENT_STEPS
                : remaining_steps;

        /*
         * ================================================================
         * 🔄 EXECUTA SEGMENTO
         * ================================================================
         */
        bool success = motor_motion_execute_segment(
            profile->direction,
            segment_steps,
            profile->steps,
            executed_steps,
            profile->use_ramp,
            stop_requested
        );

        if (!success) {
            break;
        }

        executed_steps += segment_steps;

        /*
         * ================================================================
         * ⚙️ COMPENSAÇÃO MECÂNICA
         * ================================================================
         */
        if (profile->anti_stiction) {

            motor_motion_anti_stiction(
                profile->direction
            );
        }
    }

    /*
     * =====================================================================
     * 🔌 HOLD TORQUE
     * =====================================================================
     */
#if MOTOR_HOLD_ENABLE

    esp_rom_delay_us(
        MOTOR_HOLD_TIMEOUT_MS * 1000
    );

#endif

    /*
     * =====================================================================
     * 🔌 DESLIGA BOBINAS
     * =====================================================================
     */
    motor_hw_coils_off();
}