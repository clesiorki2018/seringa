#ifndef SERINGA_H
#define SERINGA_H

#include <stdbool.h>
#include <stdint.h>

/*
 * ============================================================================
 * 📊 ESTADO DA SERINGA
 * ============================================================================
 */
typedef enum {

    SERINGA_IDLE = 0,

    SERINGA_MOVING,

    SERINGA_CHEIA,

    SERINGA_VAZIA,

    SERINGA_ERROR

} seringa_status_t;

/*
 * ============================================================================
 * ⚙️ PERFIL DE FLUXO
 * ============================================================================
 *
 * Define comportamento mecânico/hidráulico.
 * ============================================================================
 */
typedef enum {

    /*
     * Movimento extremamente suave.
     *
     * Ideal:
     *  - alta viscosidade
     *  - precisão
     *  - microdosagem
     */
    SERINGA_FLOW_PRECISION = 0,

    /*
     * Perfil balanceado.
     */
    SERINGA_FLOW_NORMAL,

    /*
     * Máxima suavidade.
     */
    SERINGA_FLOW_SMOOTH,

    /*
     * Prioriza velocidade.
     */
    SERINGA_FLOW_FAST

} seringa_flow_profile_t;

/*
 * ============================================================================
 * 🚀 INICIALIZAÇÃO
 * ============================================================================
 */
void seringa_init(void);

/*
 * ============================================================================
 * 💉 INJEÇÃO
 * ============================================================================
 *
 * Movimento controlado com:
 *  - rampa
 *  - compensação mecânica
 *  - perfil hidráulico
 * ============================================================================
 */
bool seringa_injetar_ml(
    float ml,
    seringa_flow_profile_t profile
);

/*
 * ============================================================================
 * ♻️ RECARREGAMENTO
 * ============================================================================
 */
bool seringa_recarregar_ml(
    float ml,
    seringa_flow_profile_t profile
);

/*
 * ============================================================================
 * 🧪 RECARGA TOTAL
 * ============================================================================
 *
 * Vai até endstop traseiro.
 * ============================================================================
 */
bool seringa_encher_total(void);

/*
 * ============================================================================
 * 🛑 STOP
 * ============================================================================
 */
void seringa_stop(void);

/*
 * ============================================================================
 * 📊 STATUS
 * ============================================================================
 */
seringa_status_t seringa_get_status(void);

bool seringa_is_busy(void);

bool seringa_is_cheia(void);

bool seringa_is_vazia(void);

/*
 * ============================================================================
 * ⚙️ CALIBRAÇÃO
 * ============================================================================
 */
void seringa_set_steps_por_ml(float value);

float seringa_get_steps_por_ml(void);

#endif