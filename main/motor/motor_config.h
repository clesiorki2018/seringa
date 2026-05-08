#ifndef MOTOR_CONFIG_H
#define MOTOR_CONFIG_H

/*
 * ============================================================================
 * 🔧 CONFIGURAÇÃO DE HARDWARE
 * ============================================================================
 *
 * ULN2003 + 28BYJ-48
 *
 * IMPORTANTE:
 *  - Alterar GPIOs SOMENTE aqui
 *  - Nunca espalhar GPIO pelo projeto
 * ============================================================================
 */

#define MOTOR_GPIO_IN1              18
#define MOTOR_GPIO_IN2              19
#define MOTOR_GPIO_IN3              21
#define MOTOR_GPIO_IN4              22

/*
 * ============================================================================
 * 🔴 FINS DE CURSO
 * ============================================================================
 *
 * Sensores ativos em LOW.
 * ============================================================================
 */

#define MOTOR_ENDSTOP_BACK_GPIO     17
#define MOTOR_ENDSTOP_FRONT_GPIO    16

/*
 * ============================================================================
 * 🔄 DIREÇÃO GLOBAL
 * ============================================================================
 *
 * Seu sistema mecânico foi montado invertido.
 *
 * Ao ativar:
 *  FORWARD  <-> BACKWARD
 *
 * Isso evita:
 *  - gambiarra espalhada
 *  - sinais negativos
 *  - inversões inconsistentes
 * ============================================================================
 */

#define MOTOR_DIRECTION_INVERTED    1

/*
 * ============================================================================
 * ⏱️ VELOCIDADE / TIMING
 * ============================================================================
 *
 * 28BYJ-48 NÃO gosta de velocidades muito altas com carga axial.
 *
 * IMPORTANTE:
 *  - menor delay = maior velocidade
 *  - velocidade excessiva:
 *      -> vibração
 *      -> perda de passo
 *      -> stick-slip
 *      -> torque baixo
 *
 * Recomendado para:
 *  - fuso 5mm
 *  - acoplador flexível
 *  - seringa
 * ============================================================================
 */

/*
 * Delay inicial da rampa.
 *
 * Partida suave:
 *  - reduz tranco
 *  - reduz torção do acoplador
 */
#define MOTOR_RAMP_START_DELAY_US       3000

/*
 * Delay mínimo.
 *
 * Velocidade máxima operacional.
 */
#define MOTOR_RAMP_MIN_DELAY_US         1200

/*
 * Quantidade de passos usados para:
 *  - aceleração
 *  - desaceleração
 */
#define MOTOR_RAMP_STEPS                128

/*
 * ============================================================================
 * 🧠 SEGMENTAÇÃO DE MOVIMENTO
 * ============================================================================
 *
 * Movimento longo é dividido em blocos.
 *
 * Benefícios:
 *  - STOP mais responsivo
 *  - menor acúmulo de erro
 *  - permite compensações mecânicas
 * ============================================================================
 */

#define MOTOR_SEGMENT_STEPS             64

/*
 * ============================================================================
 * ⚙️ ANTI-STICK-SLIP / ANTI-STICTION
 * ============================================================================
 *
 * Estratégia inspirada em:
 *  - CNC peck drilling
 *  - extrusoras
 *  - bombas industriais
 *
 * Objetivo:
 *  - reduzir solavancos
 *  - aliviar pressão acumulada
 *  - reduzir atrito estático
 *
 * Funcionamento:
 *
 *      +64 passos
 *      -1 passo
 *      +64 passos
 *      -1 passo
 *
 * ============================================================================
 */

#define MOTOR_ANTI_STICTION_ENABLE      1

/*
 * Quantidade de passos entre compensações.
 */
#define MOTOR_ANTI_STICTION_INTERVAL    64

/*
 * Micro retração.
 *
 * NÃO usar valores altos.
 */
#define MOTOR_ANTI_STICTION_BACKSTEPS   1

/*
 * ============================================================================
 * 🔌 HOLD TORQUE
 * ============================================================================
 *
 * Mantém bobinas energizadas após movimento.
 *
 * Benefícios:
 *  - reduz backlash
 *  - mantém pressão
 *
 * Desvantagens:
 *  - aquece motor
 *  - consome energia
 * ============================================================================
 */

#define MOTOR_HOLD_ENABLE               1

/*
 * Tempo mantendo torque após parar.
 */
#define MOTOR_HOLD_TIMEOUT_MS           1500

/*
 * ============================================================================
 * 🔴 DEBOUNCE DOS ENDSTOPS
 * ============================================================================
 */

#define MOTOR_ENDSTOP_CONFIRMATIONS     5

#endif