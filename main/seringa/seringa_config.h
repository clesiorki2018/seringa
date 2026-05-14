#ifndef SERINGA_CONFIG_H
#define SERINGA_CONFIG_H

/*
 * ============================================================================
 * ⚙️ CONFIGURAÇÕES MECÂNICAS
 * ============================================================================
 */

/*
 * Quantidade máxima de passos enviados
 * numa única operação segmentada.
 */
#define SERINGA_SEGMENT_STEPS 1280

/*
 * ============================================================================
 * ⚙️ COMPENSAÇÃO HIDRÁULICA
 * ============================================================================
 *
 * Inspirado em:
 *  - peck drilling CNC
 *  - extrusoras
 *  - bombas industriais
 *
 * Objetivo:
 *  - aliviar pressão
 *  - reduzir stick-slip
 *  - reduzir trancos
 */

/*
 * Habilita micro retração.
 */
#define SERINGA_PRESSURE_RELIEF_ENABLE 1

/*
 * Quantidade de micro-passos de alívio.
 *
 * IMPORTANTE:
 *  - pequeno demais -> sem efeito
 *  - grande demais -> refluxo
 *
 * Comece com:
 *  1~3
 */
#define SERINGA_PRESSURE_RELIEF_STEPS 20

/*
 * Intervalo entre compensações.
 *
 * Exemplo:
 *  a cada 128 passos:
 *      -> retrai 2
 *      -> continua
 */
#define SERINGA_PRESSURE_RELIEF_INTERVAL 128

/*
 * ============================================================================
 * ⚙️ PERFIS DE FLUXO
 * ============================================================================
 */

/*
 * Precision:
 *  - máxima suavidade
 *  - mais lento
 */
#define SERINGA_PRECISION_RAMP true
#define SERINGA_PRECISION_ANTI_STICTION true

/*
 * Smooth:
 */
#define SERINGA_SMOOTH_RAMP true
#define SERINGA_SMOOTH_ANTI_STICTION true

/*
 * Normal:
 */
#define SERINGA_NORMAL_RAMP true
#define SERINGA_NORMAL_ANTI_STICTION false

/*
 * Fast:
 */
#define SERINGA_FAST_RAMP false
#define SERINGA_FAST_ANTI_STICTION false

#endif