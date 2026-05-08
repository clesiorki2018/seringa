#ifndef SERINGA_MOTION_H
#define SERINGA_MOTION_H

#include "seringa.h"

#include <stdbool.h>

/*
 * ============================================================================
 * 💉 EXECUTA INJEÇÃO
 * ============================================================================
 *
 * Responsável por:
 *  - converter ml -> steps
 *  - configurar perfil do motor
 *  - aplicar compensação hidráulica
 */
bool seringa_motion_injetar(
    float ml,
    seringa_flow_profile_t profile
);

/*
 * ============================================================================
 * ♻️ EXECUTA RECARGA
 * ============================================================================
 */
bool seringa_motion_recarregar(
    float ml,
    seringa_flow_profile_t profile
);

/*
 * ============================================================================
 * 🧪 ENCHIMENTO TOTAL
 * ============================================================================
 */
bool seringa_motion_encher_total(void);

#endif