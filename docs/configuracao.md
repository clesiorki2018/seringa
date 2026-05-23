# Configuração

## Segredos

Crie `.env` na raiz do projeto a partir de `.env.example`.

| Variável | Uso |
| --- | --- |
| `SERINGA_WIFI_SSID` | Nome da rede WiFi usada pelo ESP32. |
| `SERINGA_WIFI_PASS` | Senha da rede WiFi. |
| `SERINGA_PIN_HASH` | Hash DJB2 do PIN de login web, em hexadecimal. |

O arquivo `.env` não deve ser versionado.

## Gerar Hash do PIN

Exemplo para PIN `1234`:

```sh
python3 -c 'h=5381
for c in "1234": h=((h << 5) + h + ord(c)) & 0xffffffff
print(f"0x{h:08x}")'
```

Copie a saída para `SERINGA_PIN_HASH`.

## Build

O CMake lê `.env` automaticamente. Como alternativa, exporte as variáveis no shell antes do build:

```sh
export SERINGA_WIFI_SSID="nome-da-rede"
export SERINGA_WIFI_PASS="senha-da-rede"
export SERINGA_PIN_HASH="0x00000000"
idf.py build
```

## Hardware

Os parâmetros de hardware ficam centralizados em `main/motor/motor_config.h`.

### Mapa GPIO atual

| Função | GPIO | Observação |
| --- | ---: | --- |
| Motor | - | 28BYJ-48 |
| Driver | - | ULN2003 |
| Motor IN1 | 18 | Saída para ULN2003 |
| Motor IN2 | 19 | Saída para ULN2003 |
| Motor IN3 | 21 | Saída para ULN2003 |
| Motor IN4 | 22 | Saída para ULN2003 |
| Fim de curso traseiro / cheia | 27 | Entrada ativa em HIGH, pulldown interno |
| Fim de curso dianteiro / vazia | 25 | Entrada ativa em HIGH, pulldown interno |
| Botões físicos | - | Temporariamente removidos do firmware |

### Ligações dos acionamentos

Fins de curso usam acionamento em HIGH:

```text
3V3/sinal HIGH ---- sensor ---- GPIO
GPIO ---- resistor pulldown ---- GND
```

GPIO25 e GPIO27 possuem pulldown interno disponível no ESP32.

### Diagrama de pinagem

Diagrama lógico da ligação atual:

```text
                         ESP32 / ESP-WROOM-32

                 GPIO 25 ───── fim de curso VAZIA ────── HIGH ao acionar
                 GPIO 27 ───── fim de curso CHEIA ────── HIGH ao acionar

                 GPIO 18 ───── ULN2003 IN1
                 GPIO 19 ───── ULN2003 IN2
                 GPIO 21 ───── ULN2003 IN3
                 GPIO 22 ───── ULN2003 IN4

                    GND ────── GND do ULN2003
                    5V  ────── VCC do ULN2003/motor
```

Mapa resumido:

```text
                 +---------------------------+
                 |          ESP32            |
                 |                           |
 Endstop vazio ->| GPIO25              GPIO18|---> ULN2003 IN1
 Endstop cheio ->| GPIO27              GPIO19|---> ULN2003 IN2
                 |                     GPIO21|---> ULN2003 IN3
                 |                     GPIO22|---> ULN2003 IN4
                 | GND ---------------- GND  |
                 +---------------------------+
```

### Comportamento dos botões

Botões físicos estão temporariamente removidos do firmware. Os comandos devem ser feitos pela interface web/API.

### Opções de hardware

| Item | Valor atual |
| --- | --- |
| Fins de curso instalados | `MOTOR_ENDSTOPS_INSTALLED 1` |
| Direção invertida | `MOTOR_DIRECTION_INVERTED 1` |

> Nota: use a shield apenas para alimentação/distribuição e evite GPIOs críticos no conector da shield. Adicione um capacitor de 470 µF a 1000 µF na alimentação do motor e um capacitor de 100 nF próximo ao driver ULN2003.
