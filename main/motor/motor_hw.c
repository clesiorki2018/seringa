#include "motor_hw.h"
#include "motor_config.h"

#include "driver/gpio.h"

#include <stddef.h>

/*
 * ============================================================================
 * 🧠 ESTADO INTERNO
 * ============================================================================
 *
 * Índice atual da sequência half-step.
 *
 * Mantido privado nesta unidade.
 * ============================================================================
 */
static uint8_t g_step_index = 0;

/*
 * ============================================================================
 * 🔄 SEQUÊNCIA HALF-STEP
 * ============================================================================
 *
 * Sequência otimizada para:
 *  - 28BYJ-48
 *  - ULN2003
 *
 * Ordem:
 *  IN1
 *  IN2
 *  IN3
 *  IN4
 *
 * IMPORTANTE:
 *  - Alterar sequência incorretamente pode:
 *      -> vibrar
 *      -> inverter
 *      -> perder torque
 * ============================================================================
 */
static const uint8_t g_halfstep_sequence[8][4] = {

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
 * 🔌 APLICA ESTADO NAS BOBINAS
 * ============================================================================
 */
static inline void motor_hw_apply_phase(uint8_t phase)
{
    gpio_set_level(MOTOR_GPIO_IN1, g_halfstep_sequence[phase][0]);
    gpio_set_level(MOTOR_GPIO_IN2, g_halfstep_sequence[phase][1]);
    gpio_set_level(MOTOR_GPIO_IN3, g_halfstep_sequence[phase][2]);
    gpio_set_level(MOTOR_GPIO_IN4, g_halfstep_sequence[phase][3]);
}

/*
 * ============================================================================
 * 🧠 DEBOUNCE DOS ENDSTOPS
 * ============================================================================
 *
 * Estratégia:
 *  - múltiplas leituras rápidas
 *  - sem delays bloqueantes
 * ============================================================================
 */
static bool motor_hw_confirm_endstop(gpio_num_t pin)
{
    uint32_t confirmations = 0;

    for (uint32_t i = 0;
         i < MOTOR_ENDSTOP_CONFIRMATIONS;
         i++) {

        if (gpio_get_level(pin) == 0) {
            confirmations++;
        }
    }

    return confirmations == MOTOR_ENDSTOP_CONFIRMATIONS;
}

/*
 * ============================================================================
 * 🚀 INICIALIZAÇÃO
 * ============================================================================
 */
void motor_hw_init(void)
{
    /*
     * 🔌 GPIOs DO MOTOR
     */
    gpio_config_t motor_gpio_config = {

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

    gpio_config(&motor_gpio_config);

    /*
     * 🔴 GPIOs DOS ENDSTOPS
     */
    gpio_config_t endstop_gpio_config = {

        .pin_bit_mask =
            (1ULL << MOTOR_ENDSTOP_FRONT_GPIO) |
            (1ULL << MOTOR_ENDSTOP_BACK_GPIO),

        .mode = GPIO_MODE_INPUT,

        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,

        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&endstop_gpio_config);

    motor_hw_coils_off();
}

/*
 * ============================================================================
 * 🔄 EXECUTA UM PASSO
 * ============================================================================
 */
void motor_hw_step(bool direction)
{
    /*
     * =========================================================================
     * 🔄 INVERSÃO GLOBAL DE DIREÇÃO
     * =========================================================================
     *
     * Corrige montagem mecânica invertida.
     * =========================================================================
     */
#if MOTOR_DIRECTION_INVERTED
    direction = !direction;
#endif

    /*
     * =========================================================================
     * 🔄 AVANÇA ÍNDICE DA SEQUÊNCIA
     * =========================================================================
     */
    if (direction) {

        g_step_index++;

        if (g_step_index >= 8) {
            g_step_index = 0;
        }

    } else {

        if (g_step_index == 0) {
            g_step_index = 7;
        } else {
            g_step_index--;
        }
    }

    /*
     * =========================================================================
     * 🔌 APLICA FASE
     * =========================================================================
     */
    motor_hw_apply_phase(g_step_index);
}

/*
 * ============================================================================
 * 🔌 DESLIGA BOBINAS
 * ============================================================================
 */
void motor_hw_coils_off(void)
{
    gpio_set_level(MOTOR_GPIO_IN1, 0);
    gpio_set_level(MOTOR_GPIO_IN2, 0);
    gpio_set_level(MOTOR_GPIO_IN3, 0);
    gpio_set_level(MOTOR_GPIO_IN4, 0);
}

/*
 * ============================================================================
 * 🔴 ENDSTOPS
 * ============================================================================
 */

bool motor_hw_front_endstop_triggered(void)
{
    return motor_hw_confirm_endstop(MOTOR_ENDSTOP_FRONT_GPIO);
}

bool motor_hw_back_endstop_triggered(void)
{
    return motor_hw_confirm_endstop(MOTOR_ENDSTOP_BACK_GPIO);
}