#ifndef SERINGA_H
#define SERINGA_H

#include <stdbool.h>

/*
 * ============================================================================
 * 🧠 SERINGA - API PÚBLICA DO DOMÍNIO
 * ============================================================================
 *
 * Responsabilidades:
 *
 *  - expor comandos de alto nível
 *  - proteger movimentos inválidos
 *  - consultar status lógico da seringa
 *
 * NÃO deve conter:
 *
 *  - GPIO
 *  - HTTP
 *  - NVS
 *  - controle direto de bobinas
 * ============================================================================
 */

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
 */
typedef enum {

    SERINGA_FLOW_PRECISION = 0,
    SERINGA_FLOW_NORMAL,
    SERINGA_FLOW_SMOOTH,
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
 * 💉 INJEÇÃO / RECARGA
 * ============================================================================
 */
bool seringa_injetar_ml(
    float ml,
    seringa_flow_profile_t profile
);

bool seringa_recarregar_ml(
    float ml,
    seringa_flow_profile_t profile
);

/*
 * ============================================================================
 * 🧪 ENCHIMENTO TOTAL
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

#endif