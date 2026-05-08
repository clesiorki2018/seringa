#ifndef MOTOR_HW_H
#define MOTOR_HW_H

#include <stdbool.h>
#include <stdint.h>

/*
 * ============================================================================
 * 🔧 MOTOR HW - CAMADA DE HARDWARE
 * ============================================================================
 *
 * Responsabilidades:
 *
 *  - Controle direto das bobinas
 *  - Sequência half-step
 *  - GPIO
 *  - Leitura dos fins de curso
 *
 * NÃO deve conter:
 *  - FreeRTOS
 *  - filas
 *  - lógica da seringa
 *  - rampas
 *  - regras de negócio
 *
 * Esta camada deve ser:
 *  - rápida
 *  - determinística
 *  - simples
 * ============================================================================
 */

/*
 * ============================================================================
 * 🚀 INICIALIZAÇÃO
 * ============================================================================
 *
 * Configura:
 *  - GPIOs do ULN2003
 *  - GPIOs dos endstops
 */
void motor_hw_init(void);

/*
 * ============================================================================
 * 🔄 EXECUTA UM HALF-STEP
 * ============================================================================
 *
 * direction:
 *  true  -> forward
 *  false -> backward
 *
 * IMPORTANTE:
 *  - executa APENAS um passo
 *  - NÃO possui delays
 *  - NÃO possui rampas
 */
void motor_hw_step(bool direction);

/*
 * ============================================================================
 * 🔌 DESLIGA BOBINAS
 * ============================================================================
 *
 * Remove corrente do motor.
 */
void motor_hw_coils_off(void);

/*
 * ============================================================================
 * 🔴 ENDSTOPS
 * ============================================================================
 */

bool motor_hw_front_endstop_triggered(void);

bool motor_hw_back_endstop_triggered(void);

#endif