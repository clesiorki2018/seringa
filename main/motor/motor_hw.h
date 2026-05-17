/*
 * Copyright 2026 clesiorki
 *
 * Licensed under the Apache License, Version 2.0.
 * See the LICENSE file in the project root for full license information.
 */

#ifndef MOTOR_HW_H
#define MOTOR_HW_H

#include <stdbool.h>
#include <stdint.h>

/*
 * ============================================================================
 * 🔌 MOTOR HW LAYER
 * ============================================================================
 *
 * Esta camada é responsável EXCLUSIVAMENTE pelo hardware.
 *
 * Responsabilidades:
 *
 *  - GPIO
 *  - bobinas
 *  - endstops
 *  - energização
 *  - stepping físico
 *
 * NÃO deve conter:
 *
 *  - lógica de negócio
 *  - filas
 *  - FreeRTOS
 *  - aceleração
 *  - web
 *  - seringa
 *
 * Arquitetura:
 *
 *   motor.c
 *      ↓
 *   motor_motion.c
 *      ↓
 *   motor_hw.c
 *      ↓
 *   GPIO / ULN2003 / Endstops
 *
 * ============================================================================
 */

/*
 * ============================================================================
 * 🚀 INICIALIZAÇÃO
 * ============================================================================
 *
 * Configura:
 *
 *  - GPIOs do ULN2003
 *  - GPIOs dos endstops
 *  - pull-ups internos
 *
 * Segurança:
 *
 *  - inicia com bobinas desligadas
 * ============================================================================
 */
void motor_hw_init(void);

/*
 * ============================================================================
 * 🔄 APLICA HALF-STEP
 * ============================================================================
 *
 * Executa um estado da sequência half-step.
 *
 * step_index:
 *
 *  0 → 7
 *
 * IMPORTANTE:
 *
 *  - chamada em tempo real
 *  - extremamente crítica para timing
 * ============================================================================
 */
void motor_hw_apply_step(
    uint8_t step_index
);

/*
 * ============================================================================
 * 🔌 DESLIGA BOBINAS
 * ============================================================================
 *
 * Remove energização do motor.
 *
 * Benefícios:
 *
 *  - reduz aquecimento
 *  - reduz consumo
 *  - evita desgaste
 * ============================================================================
 */
void motor_hw_coils_off(void);

/*
 * ============================================================================
 * 🔴 ENDSTOP FRONTAL
 * ============================================================================
 *
 * Retorna:
 *
 *  true  -> acionado
 *  false -> livre
 *
 * IMPORTANTE:
 *
 *  - ativo em LOW
 * ============================================================================
 */
bool motor_hw_front_endstop_triggered(void);

/*
 * ============================================================================
 * 🔴 ENDSTOP TRASEIRO
 * ============================================================================
 *
 * Retorna:
 *
 *  true  -> acionado
 *  false -> livre
 *
 * IMPORTANTE:
 *
 *  - ativo em LOW
 * ============================================================================
 */
bool motor_hw_back_endstop_triggered(void);

#endif