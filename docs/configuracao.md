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

| Item | Valor atual |
| --- | --- |
| Motor | 28BYJ-48 |
| Driver | ULN2003 |
| GPIO IN1 | 18 |
| GPIO IN2 | 19 |
| GPIO IN3 | 21 |
| GPIO IN4 | 22 |
| Fim de curso traseiro | GPIO 17 |
| Fim de curso dianteiro | GPIO 16 |
| Botão injetar 1 ml | GPIO 25 |
| Botão recarregar até fim traseiro | GPIO 26 |
| Fins de curso instalados | `MOTOR_ENDSTOPS_INSTALLED 1` |
| Direção invertida | `MOTOR_DIRECTION_INVERTED 1` |
