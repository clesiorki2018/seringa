#ifndef MOTOR_MOTION_H
#define MOTOR_MOTION_H

#include <stdbool.h>
#include <stdint.h>

/*
 * ============================================================================
 * 🧠 PERFIL DE MOVIMENTO
 * ============================================================================
 *
 * Define como o movimento será executado.
 *
 * Objetivo:
 *  - desacoplar movimento da lógica de negócio
 *  - permitir diferentes perfis mecânicos
 *  - facilitar tuning fino
 * ============================================================================
 */
typedef struct {

    /*
     * Direção:
     *  true  -> forward
     *  false -> backward
     */
    bool direction;

    /*
     * Quantidade total de passos.
     */
    uint32_t steps;

    /*
     * Habilita rampa trapezoidal.
     */
    bool use_ramp;

    /*
     * Habilita compensação anti-stick-slip.
     */
    bool anti_stiction;

} motor_motion_profile_t;

/*
 * ============================================================================
 * 🚀 EXECUTA MOVIMENTO
 * ============================================================================
 *
 * Função BLOQUEANTE.
 *
 * IMPORTANTE:
 *  - NÃO criar task aqui
 *  - NÃO usar fila aqui
 *  - Apenas executar movimento
 *
 * stop_requested:
 *  - ponteiro para flag global de cancelamento
 * ============================================================================
 */
void motor_motion_execute(
    const motor_motion_profile_t *profile,
    volatile bool *stop_requested
);

#endif