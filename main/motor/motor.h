#ifndef MOTOR_H
#define MOTOR_H

#include <stdbool.h>
#include <stdint.h>

/*
 * ============================================================================
 * 🧠 MOTOR - INTERFACE PÚBLICA
 * ============================================================================
 *
 * Camada responsável por:
 *
 *  - Orquestrar movimentos assíncronos
 *  - Gerenciar fila interna
 *  - Controlar estados do motor
 *  - Executar perfis de movimento
 *
 * IMPORTANTE:
 *  - NÃO contém lógica da seringa
 *  - NÃO trabalha com ml
 *  - NÃO conhece regras de negócio
 *
 * O motor executa apenas movimentos físicos.
 *
 * Arquitetura:
 *
 *   seringa
 *      ↓
 *   motor
 *      ↓
 *   motor_motion
 *      ↓
 *   motor_hw
 *      ↓
 *   GPIO / ULN2003
 *
 * ============================================================================
 */

/*
 * ============================================================================
 * 🔄 DIREÇÃO DE MOVIMENTO
 * ============================================================================
 */
typedef enum {

    /*
     * Avança eixo.
     */
    MOTOR_DIRECTION_FORWARD = 0,

    /*
     * Recua eixo.
     */
    MOTOR_DIRECTION_BACKWARD

} motor_direction_t;

/*
 * ============================================================================
 * 📊 ESTADO DO MOTOR
 * ============================================================================
 */
typedef enum {

    /*
     * Motor parado.
     */
    MOTOR_STATE_IDLE = 0,

    /*
     * Movimento em execução.
     */
    MOTOR_STATE_RUNNING,

    /*
     * Processo de parada.
     */
    MOTOR_STATE_STOPPING,

    /*
     * Estado de erro.
     */
    MOTOR_STATE_ERROR

} motor_state_t;

/*
 * ============================================================================
 * ⚙️ CONFIGURAÇÃO DE MOVIMENTO
 * ============================================================================
 *
 * Define como o movimento será executado.
 *
 * Objetivo:
 *  - desacoplar perfil mecânico
 *  - permitir tuning fino
 *  - permitir perfis futuros
 * ============================================================================
 */
typedef struct {

    /*
     * Direção do movimento.
     */
    motor_direction_t direction;

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
    bool anti_stiction_enable;

} motor_move_t;

/*
 * ============================================================================
 * 🚀 INICIALIZAÇÃO
 * ============================================================================
 *
 * Responsável por:
 *
 *  - inicializar hardware
 *  - criar fila
 *  - criar task
 *  - preparar estados internos
 *
 * Deve ser chamado apenas uma vez.
 * ============================================================================
 */
void motor_init(void);

/*
 * ============================================================================
 * 🚀 EXECUTA MOVIMENTO ASSÍNCRONO
 * ============================================================================
 *
 * Envia movimento para a fila interna.
 *
 * IMPORTANTE:
 *  - NÃO bloqueia
 *  - movimento executa em task separada
 *
 * Retorno:
 *  - true  -> comando aceito
 *  - false -> erro / ocupado / fila cheia
 *
 * Segurança:
 *  - rejeita movimento inválido
 *  - rejeita comandos concorrentes
 * ============================================================================
 */
bool motor_move(const motor_move_t *move);

/*
 * ============================================================================
 * 🛑 PARADA IMEDIATA
 * ============================================================================
 *
 * Solicita interrupção imediata.
 *
 * IMPORTANTE:
 *  - assíncrono
 *  - monitorado em tempo real
 *  - resposta quase imediata
 * ============================================================================
 */
void motor_stop(void);

/*
 * ============================================================================
 * 📊 STATUS
 * ============================================================================
 */


bool motor_is_running(void);

/*
 * ============================================================================
 * 🔴 ENDSTOPS
 * ============================================================================
 */

bool motor_front_endstop_triggered(void);

bool motor_back_endstop_triggered(void);

motor_state_t motor_get_state(void);

#endif