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
| Fim de curso traseiro | 17 | Entrada ativa em LOW, pull-up interno |
| Fim de curso dianteiro | 16 | Entrada ativa em LOW, pull-up interno |
| Botão injetar 1 ml | 25 | Entrada ativa em LOW, pull-up interno |
| Botão recarregar até fim traseiro | 26 | Entrada ativa em LOW, pull-up interno |

### Ligações dos acionamentos

Botões e fins de curso usam a mesma lógica elétrica:

```text
GPIO ---- botão/sensor ---- GND
```

Em repouso a leitura fica HIGH pelo pull-up interno. Quando acionado, o GPIO vai para LOW.

### Diagrama de pinagem

Diagrama lógico da ligação atual:

```text
                         ESP32 / ESP-WROOM-32

                 GPIO 25 ───── botão INJETAR 1 ml ───── GND
                 GPIO 26 ───── botão RECARREGAR ──────── GND

                 GPIO 16 ───── fim de curso DIANTEIRO ── GND
                 GPIO 17 ───── fim de curso TRASEIRO ─── GND

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
 Botão 1 ml  --->| GPIO25              GPIO18|---> ULN2003 IN1
 Botão fill  --->| GPIO26              GPIO19|---> ULN2003 IN2
                 |                     GPIO21|---> ULN2003 IN3
                 |                     GPIO22|---> ULN2003 IN4
 Endstop front ->| GPIO16                   |
 Endstop back  ->| GPIO17                   |
                 | GND ---------------- GND  |
                 +---------------------------+
```

### Comportamento dos botões

| Botão | Comportamento |
| --- | --- |
| Injetar 1 ml | Um pressionamento inicia a injeção de 1 ml. |
| Recarregar | O motor recarrega somente enquanto o botão estiver pressionado, parando ao soltar ou ao atingir o fim de curso traseiro. |

### Opções de hardware

| Item | Valor atual |
| --- | --- |
| Fins de curso instalados | `MOTOR_ENDSTOPS_INSTALLED 1` |
| Direção invertida | `MOTOR_DIRECTION_INVERTED 1` |
