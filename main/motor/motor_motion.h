#ifndef MOTOR_MOTION_H
#define MOTOR_MOTION_H

#include <stdbool.h>
#include <stdint.h>

/*
 * ============================================================================
 * ⚙️ MOTION ENGINE
 * ============================================================================
 *
 * Este módulo implementa:
 *
 *  - stepping
 *  - ramp-up/down
 *  - controle temporal
 *  - anti-stiction
 *  - compensação mecânica
 *  - monitoramento de endstop
 *  - parada cooperativa
 *
 * IMPORTANTE:
 *
 * Esta camada NÃO conhece:
 *  - web
 *  - seringa
 *  - ml
 *  - HTTP
 *  - calibração
 *
 * Ela apenas executa movimento.
 *
 * Arquitetura:
 *
 *   motor.c
 *      ↓
 *   motor_motion.c
 *      ↓
 *   motor_hw.c
 *
 * ============================================================================
 */

/*
 * ============================================================================
 * 📦 PERFIL DE MOVIMENTO
 * ============================================================================
 *
 * direction:
 *  true  -> frente
 *  false -> trás
 *
 * steps:
 *  quantidade total de passos
 *
 * use_ramp:
 *  habilita aceleração/desaceleração
 *
 * anti_stiction:
 *  ativa compensação mecânica periódica
 * ============================================================================
 */
typedef struct {

    bool direction;

    uint32_t steps;

    bool use_ramp;

    bool anti_stiction;

} motor_motion_profile_t;

/*
 * ============================================================================
 * ⚙️ EXECUÇÃO DE MOVIMENTO
 * ============================================================================
 *
 * Executa um movimento COMPLETO.
 *
 * IMPORTANTE:
 *  - função bloqueante
 *  - chamada pela motor_task
 *  - alta previsibilidade temporal
 *
 * Segurança:
 *  - monitora stop_requested
 *  - monitora endstops
 *  - interrompe imediatamente se necessário
 *
 * profile:
 *  perfil de movimento
 *
 * stop_requested:
 *  flag compartilhada
 *  monitorada em tempo real
 * ============================================================================
 */
void motor_motion_execute(
    const motor_motion_profile_t *profile,
    volatile bool *stop_requested
);

#endif