# Seringa Automatizada

Firmware ESP-IDF para controle de uma seringa automatizada com motor de passo, fins de curso opcionais, calibração persistida em NVS e interface web embarcada.

## Visão Geral

O sistema inicializa o ESP32, configura storage em NVS, carrega a calibração, sincroniza o domínio da seringa, conecta no WiFi e publica uma interface web pelo servidor HTTP interno do ESP-IDF.

Módulos principais:

- `main/motor`: controle assíncrono do motor, GPIOs, rampa, segmentos e fins de curso.
- `main/seringa`: regras de domínio para injetar, recarregar, encher e parar.
- `main/calibration`: leitura, validação e persistência de `steps_per_ml`.
- `main/storage`: fachada de persistência sobre NVS.
- `main/web`: servidor HTTP, autenticação, rotas estáticas e API.
- `main/wifi`: conexão WiFi em modo station.
- `data`: interface web gravada na partição SPIFFS.

## Hardware

Este firmware é projetado para o seguinte conjunto de hardware:

- ESP32 WROOM DevKit 38 pinos
- Shield/placa de expansão ESP32 38 pinos
- Motor de passo 28BYJ-48 com driver ULN2003
- Motor IN1 = GPIO 18
- Motor IN2 = GPIO 19
- Motor IN3 = GPIO 21
- Motor IN4 = GPIO 22
- Endstop vazio = GPIO 26, ativo em HIGH com pulldown interno
- Endstop cheio = GPIO 27, ativo em HIGH com pulldown interno
- Botões físicos temporariamente removidos do firmware
- Target ESP-IDF: `esp32`

### Cuidados elétricos

- Use a shield apenas para distribuição de energia e pontos de conexão não críticos.
- Evite usar GPIOs do shield para sinais críticos de controle do motor.
- Recomenda-se capacitor eletrolítico de 470 µF a 1000 µF na alimentação do motor.
- Adicione também um capacitor de desacoplamento de 100 nF próximo ao ULN2003/driver.

## Regras de desenvolvimento

- Use ESP-IDF, não Arduino, salvo pedido explícito.
- Código em C bem comentado, com estilo semelhante ao Linux.
- Aplique Clean Code e SOLID sempre que fizer sentido.
- Separe responsabilidades por módulos claros.
- Antes de refatorar, sugira checkpoint Git.
- Depois de alterar, sugira build, `git diff` e commit.

## Ambiente de Desenvolvimento

Este projeto usa ESP-IDF e deve funcionar em qualquer distribuição Linux
compatível com o toolchain oficial da Espressif. Os comandos abaixo usam
`~/esp` como diretório de trabalho para o framework, mas qualquer caminho
serve desde que o ambiente seja carregado corretamente.

### Dependências do Sistema

Instale Git, Python, CMake, Ninja e as bibliotecas usadas pelo ESP-IDF.

Debian, Ubuntu e derivados:

```sh
sudo apt update
sudo apt install git wget flex bison gperf python3 python3-pip python3-venv \
  cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
```

Fedora:

```sh
sudo dnf install git wget flex bison gperf python3 python3-pip python3-virtualenv \
  cmake ninja-build ccache libffi-devel openssl-devel dfu-util libusb1
```

Arch Linux:

```sh
sudo pacman -S git wget flex bison gperf python python-pip python-virtualenv \
  cmake ninja ccache libffi openssl dfu-util libusb
```

Permita acesso à porta serial do ESP32. Em Debian/Ubuntu, normalmente:

```sh
sudo usermod -aG dialout "$USER"
```

Em algumas distribuições o grupo pode ser `uucp` ou `lock`:

```sh
sudo usermod -aG uucp "$USER"
```

Depois de alterar grupos, encerre a sessão e entre novamente. Conecte a placa
e confira a porta:

```sh
ls /dev/ttyUSB* /dev/ttyACM*
```

### Instalação do ESP-IDF

Clone o ESP-IDF e instale o toolchain para ESP32:

```sh
mkdir -p ~/esp
cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32
```

Carregue o ambiente do ESP-IDF no terminal antes de usar `idf.py`:

```sh
. ~/esp/esp-idf/export.sh
```

Esse comando configura `IDF_PATH`, `PATH` e o ambiente Python criado pelo
instalador do ESP-IDF. Para não repetir manualmente, você pode adicionar um
atalho no `~/.bashrc`, `~/.zshrc` ou arquivo equivalente:

```sh
alias get_idf='. ~/esp/esp-idf/export.sh'
```

Depois abra um novo terminal e use:

```sh
get_idf
```

### Ambiente Python

O ESP-IDF já cria e usa seu próprio ambiente Python virtual em
`~/.espressif`. Para compilar este firmware, não é necessário criar um
`.venv` dentro do repositório.

Se quiser um ambiente virtual separado para scripts auxiliares locais, crie-o
fora do fluxo do ESP-IDF:

```sh
python3 -m venv .venv
. .venv/bin/activate
python -m pip install --upgrade pip
```

Antes de compilar com `idf.py`, garanta que o ambiente do ESP-IDF esteja
carregado no terminal atual:

```sh
. ~/esp/esp-idf/export.sh
```

### VS Code

Instale o VS Code e estas extensões:

- `Espressif IDF`, da Espressif Systems.
- `C/C++`, da Microsoft.
- `CMake Tools`, opcional, para navegação e integração com CMake.

Configure a extensão do ESP-IDF:

1. Abra a paleta de comandos com `Ctrl+Shift+P`.
2. Execute `ESP-IDF: Configure ESP-IDF Extension`.
3. Escolha `Use existing setup`.
4. Informe o caminho do ESP-IDF, por exemplo `~/esp/esp-idf`.
5. Selecione o Python/toolchain instalado pelo próprio ESP-IDF.

A pasta `.vscode/` é ignorada pelo Git, então configurações locais do VS Code
podem existir sem entrar no repositório.

Para compilar pelo terminal integrado do VS Code:

```sh
. ~/esp/esp-idf/export.sh
idf.py build
```

### Virtualização

O projeto pode ser usado em Linux nativo ou em uma máquina virtual Linux,
desde que a VM tenha acesso USB à placa ESP32.

Para VirtualBox, VMware, GNOME Boxes ou similares:

- habilite passthrough USB para o dispositivo serial da placa;
- adicione seu usuário aos grupos de acesso serial dentro da VM;
- confirme se a porta aparece como `/dev/ttyUSB0` ou `/dev/ttyACM0`;
- carregue o ambiente com `. ~/esp/esp-idf/export.sh` dentro da VM.

Em WSL2, a compilação costuma funcionar, mas flash e monitor dependem de
repasse USB do Windows para o Linux. Para evitar problemas de porta serial,
prefira Linux nativo ou uma VM com passthrough USB configurado.

### Clonando e Preparando o Projeto

Clone o repositório e entre na pasta:

```sh
git clone git@github.com:clesiorki2018/seringa.git
cd seringa
```

Se preferir HTTPS:

```sh
git clone https://github.com/clesiorki2018/seringa.git
cd seringa
```

Carregue o ESP-IDF e configure o alvo:

```sh
. ~/esp/esp-idf/export.sh
idf.py set-target esp32
```

## Configuração Sensível

Os dados sensíveis ficam fora do código-fonte, em `.env`.

```sh
cp .env.example .env
```

Edite:

```env
SERINGA_WIFI_SSID=nome-da-rede
SERINGA_WIFI_PASS=senha-da-rede
SERINGA_PIN_HASH=0x00000000
```

O `.env` é ignorado pelo Git. O CMake também aceita as mesmas variáveis vindas do ambiente do shell.

## Build e Flash

Com ESP-IDF carregado no ambiente:

```sh
idf.py set-target esp32
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

Troque `/dev/ttyUSB0` pela porta da sua placa, se necessário. Algumas placas
aparecem como `/dev/ttyACM0`.

A partição SPIFFS é gerada a partir de `data/` pelo `spiffs_create_partition_image`.

## Interface Web

Depois do boot, veja o IP no monitor serial. A interface web fica disponível em:

```text
http://IP_DO_ESP32/
```

O login envia o PIN em texto para `/api/login`; o firmware compara o hash DJB2 configurado por `SERINGA_PIN_HASH` e retorna um token de sessão em RAM.

## Documentação

- [Configuração](docs/configuracao.md)
- [Arquitetura](docs/arquitetura.md)
- [API HTTP](docs/api.md)
- [Arquitetura PlantUML](docs/arquitetura.puml) / [PNG](docs/arquitetura.png)
- [Casos de Uso PlantUML](docs/casos-uso.puml) / [PNG](docs/casos-uso.png)
- [Fluxo de Boot PlantUML](docs/fluxo-boot.puml) / [PNG](docs/fluxo-boot.png)
- [Fluxo Login/Comando PlantUML](docs/fluxo-login-comando.puml) / [PNG](docs/fluxo-login-comando.png)
- [Estados da Seringa PlantUML](docs/estados-seringa.puml) / [PNG](docs/estados-seringa.png)

## Licença e Marca

Este projeto é licenciado sob a [Apache License 2.0](LICENSE).

`clesiorki` é uma marca registrada no Brasil. A licença deste projeto
não concede permissão de uso da marca, exceto conforme permitido pela
própria Apache License 2.0 para identificação da origem do projeto e
reprodução dos avisos do arquivo [NOTICE](NOTICE).
