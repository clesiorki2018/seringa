#ifndef MOTOR_H
#define MOTOR_H

#include <stdbool.h>
#include <stdint.h>

/*
 * ============================================================================
 * 🧠 MOTOR - API PÚBLICA
 * ============================================================================
 *
 * Este módulo representa a camada de abstração do motor.
 *
 * Responsabilidades:
 *
 *  - interface pública do motion system
 *  - gerenciamento de fila
 *  - controle assíncrono
 *  - coordenação da motion layer
 *
 * NÃO é responsável por:
 *
 *  - GPIO
 *  - stepping
 *  - rampas
 *  - endstops físicos
 *
 * Arquitetura:
 *
 *   seringa/web/calibration
 *              ↓
 *           motor.c
 *              ↓
 *      motor_motion.c
 *              ↓
 *         motor_hw.c
 *
 * ============================================================================
 */

/*
 * ============================================================================
 * 🔄 DIREÇÃO
 * ============================================================================
 *
 * IMPORTANTE:
 *  - FORWARD/BACKWARD são semânticos
 *  - inversão mecânica é tratada internamente
 * ============================================================================
 */
typedef enum {

    /*
     * Empurra êmbolo
     */
    MOTOR_DIRECTION_FORWARD = 0,

    /*
     * Recua êmbolo
     */
    MOTOR_DIRECTION_BACKWARD

} motor_direction_t;

/*
 * ============================================================================
 * 📊 ESTADO GLOBAL
 * ============================================================================
 */
typedef enum {

    /*
     * Motor parado
     */
    MOTOR_STATE_IDLE = 0,

    /*
     * Movimento em andamento
     */
    MOTOR_STATE_RUNNING

} motor_state_t;

/*
 * ============================================================================
 * ⚙️ CONFIGURAÇÃO DE MOVIMENTO
 * ============================================================================
 *
 * Estrutura de alto nível usada pela API.
 *
 * direction:
 *  - sentido lógico do movimento
 *
 * steps:
 *  - quantidade de passos
 *
 * use_ramp:
 *  - habilita aceleração/desaceleração
 *
 * anti_stiction_enable:
 *  - ativa compensação mecânica
 *  - útil para fuso/acoplador flexível
 * ============================================================================
 */
typedef struct {

    motor_direction_t direction;

    uint32_t steps;

    bool use_ramp;

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
 *  - preparar motion engine
 *
 * Deve ser chamado UMA vez.
 * ============================================================================
 */
void motor_init(void);

/*
 * ============================================================================
 * 📡 MOVIMENTO ASSÍNCRONO
 * ============================================================================
 *
 * Agenda um movimento.
 *
 * IMPORTANTE:
 *  - NÃO bloqueia
 *  - movimento ocorre em task separada
 *  - retorna false se:
 *      - motor ocupado
 *      - fila cheia
 *      - parâmetros inválidos
 *
 * Segurança:
 *  - possui proteção contra backlog
 *  - possui validação de limites
 * ============================================================================
 */
bool motor_move(
    const motor_move_t *move
);

/*
 * ============================================================================
 * 🛑 STOP IMEDIATO
 * ============================================================================
 *
 * Solicita parada cooperativa.
 *
 * IMPORTANTE:
 *  - não mata task
 *  - motion layer monitora flag
 *  - resposta quase em tempo real
 * ============================================================================
 */
void motor_stop(void);

/*
 * ============================================================================
 * 📊 STATUS
 * ============================================================================
 */

/*
 * Retorna:
 *  - true  -> executando movimento
 *  - false -> parado
 */
bool motor_is_running(void);

/*
 * Estado detalhado
 */
motor_state_t motor_get_state(void);

/*
 * ============================================================================
 * 🔴 ENDSTOPS
 * ============================================================================
 *
 * Expostos para:
 *  - calibração
 *  - diagnóstico
 *  - homing
 *  - segurança
 *
 * IMPORTANTE:
 *  - leitura direta do hardware
 * ============================================================================
 */

/*
 * Endstop frontal acionado
 */
bool motor_front_endstop_triggered(void);

/*
 * Endstop traseiro acionado
 */
bool motor_back_endstop_triggered(void);

/*
 * Disponibilidade física dos endstops.
 *
 * Retorna:
 *  - true  -> sensores instalados e leituras válidas
 *  - false -> hardware sem sensores; leituras de endstop são forçadas livres
 *
 * IMPORTANTE:
 *  - usado por camadas de domínio para bloquear homing/enchimento total
 *  - evita que módulos acima conheçam macros de configuração do hardware
 */
bool motor_endstops_installed(void);

#endif
