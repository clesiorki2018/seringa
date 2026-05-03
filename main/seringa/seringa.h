#ifndef SERINGA_H
#define SERINGA_H

#include <stdbool.h>

/*
 * ============================================================================
 * 🧠 ESTADO DA SERINGA
 * ============================================================================
 *
 * IMPORTANTE:
 * Esse enum é a VERDADE do sistema.
 *
 * Ele é usado por:
 *  - backend (routes.c)
 *  - frontend (app.js)
 *  - lógica interna (proteções)
 *
 * NÃO altere sem atualizar todos os pontos.
 */
typedef enum {
    SERINGA_IDLE = 0,   // parada em posição intermediária
    SERINGA_MOVING,     // motor em movimento
    SERINGA_CHEIA,      // fim de curso retraído (pronta para uso)
    SERINGA_VAZIA       // fim de curso avançado (precisa recarregar)
} seringa_status_t;

/*
 * ============================================================================
 * 🚀 INICIALIZAÇÃO
 * ============================================================================
 *
 * Configura:
 *  - GPIOs dos fins de curso
 *  - estado inicial da seringa
 */
void seringa_init(void);

/*
 * ============================================================================
 * 🎯 CONTROLE EM STEPS (baixo nível)
 * ============================================================================
 *
 * Essas funções NÃO fazem conversão de ml.
 * São usadas internamente e pela calibração.
 *
 * PROTEÇÕES:
 *  - bloqueia se motor estiver rodando
 *  - respeita fins de curso
 */
void seringa_injetar_steps(int steps);      // avança êmbolo
void seringa_recarregar_steps(int steps);   // recua êmbolo

/*
 * ============================================================================
 * 🎯 CONTROLE EM ML (alto nível) [PRÓXIMO PASSO]
 * ============================================================================
 *
 * Usa fator de calibração salvo no NVS:
 *  steps_por_ml
 *
 * Ainda não implementado no .c (vamos fazer depois)
 */
void seringa_injetar_ml(float ml);
void seringa_recarregar_ml(float ml);

/*
 * ============================================================================
 * 🛑 CONTROLE GLOBAL
 * ============================================================================
 */
void seringa_stop(void);

/*
 * ============================================================================
 * 📊 STATUS
 * ============================================================================
 *
 * Sempre atualizado com base em:
 *  - fins de curso
 *  - estado do motor
 */
seringa_status_t seringa_get_status(void);

/*
 * ============================================================================
 * 🔍 HELPERS (evita duplicação de lógica)
 * ============================================================================
 */
bool seringa_is_busy(void);   // proxy de motor_is_running()

bool seringa_is_cheia(void);  // fim de curso retraído
bool seringa_is_vazia(void);  // fim de curso avançado

/*
 * ============================================================================
 * ⚙️ CALIBRAÇÃO (base)
 * ============================================================================
 *
 * steps_por_ml:
 *  - define quantos passos equivalem a 1 ml
 *  - armazenado em NVS (storage.c)
 */
void seringa_set_steps_por_ml(float value);
float seringa_get_steps_por_ml(void);

/*
 * ============================================================================
 * 🔄 AÇÕES DE SISTEMA
 * ============================================================================
 *
 * Enche completamente até fim de curso traseiro.
 * Usado na inicialização/calibração.
 */
void seringa_encher_total(void);

#endif