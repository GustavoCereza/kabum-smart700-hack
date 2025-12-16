# Kabum Smart700 Hack

Este documento descreve a lógica geral do **Kabum Smart700 Hack**, uma solução baseada em **ESP32-C3 Mini**, desenvolvida para mitigar o problema crônico de conectividade Wi-Fi do robô aspirador **Kabum Smart700 / Smash 700**.

A abordagem utiliza **supervisão elétrica externa e controle físico seguro**, sem qualquer modificação no firmware original do robô, no aplicativo ou na infraestrutura de rede.

## Visão Geral do Problema

Após alguns ciclos de limpeza, o robô pode entrar em um estado onde:

- O Wi-Fi deixa de reconectar;
- O robô continua funcionando localmente;
- O sinal Wi-Fi está disponível;
- Reinicializações via aplicativo não resolvem o problema.

Esse comportamento indica um **travamento interno do módulo de conectividade**, que não é corretamente reinicializado pelo firmware original.


## Objetivo da Solução

- Detectar estados internos do robô por leitura elétrica;
- Identificar quando o Wi-Fi está inativo em condição anômala;
- Executar um **reset físico controlado** apenas quando necessário;
- Forçar o retorno seguro do robô ao carregador;
- Operar de forma transparente e segura.


## Hardware Utilizado

- **ESP32-C3 Mini**
- 4 entradas analógicas (ADC)
- 2 saídas digitais
- Transistor NPN **BC817** (obrigatório para botão power)
- Resistor **100 Ω** (botão de carregamento)
- Watchdog por hardware (ESP-IDF)

## Arquitetura do Sistema

O ESP32-C3 Mini atua como um **supervisor externo**, monitorando sinais elétricos do robô e executando ações físicas quando uma condição de falha é detectada.

O sistema:
- Não intercepta dados;
- Não se comunica com o firmware do robô;
- Não grava memória interna;
- Atua apenas em sinais equivalentes a botões físicos e LEDs.

## Fluxo Lógico do Sistema

### Inicialização

- Configuração das entradas analógicas;
- Configuração das saídas digitais;
- ADC em 12 bits com atenuação de 11 dB;
- Inicialização opcional da serial de debug;
- Ativação do watchdog (300 s);
- Entrada em modo de monitoramento contínuo.

### Loop Principal

Executado a cada **1 segundo**, realiza:

- Leitura dos ADCs;
- Atualização dos estados internos;
- Avaliação de condições de falha;
- Execução de ações corretivas quando aplicável;
- Reset do watchdog.

## Lógica de Decisão de Reset (Implementação Real)

Na implementação real:

- A decisão de reset ocorre **dentro do loop principal**;
- O estado `awaitingCharging` atua como **bloqueio lógico temporário**;
- O reset é condicionado ao **contexto completo dos estados**, e não a uma função dedicada;
- O sistema evita múltiplos resets consecutivos por **lógica de estado**.

Ou seja, **não existe uma função única de decisão**, mas sim uma **avaliação distribuída e segura ao longo do loop**, garantindo robustez e previsibilidade.

## Leitura e Interpretação dos Sinais (ADC)

### Mapeamento de Entradas Analógicas

| GPIO ESP32-C3 | Sinal Monitorado | Descrição |
|-------------|----------------|-----------|
| A3 | LED_WIFI_G | Estado do Wi-Fi |
| A2 | LED_CHANGE_R | Estado interno de mudança |
| A1 | LED_CHANGE_G | Oscilação de carga |
| A0 | LED_POWER_G | Presença de energia / dock |

### Detecção de Wi-Fi

- **A3 < 300** → Wi-Fi ativo  
- **A3 > 2000** → Wi-Fi inativo  
- Zona intermediária é ignorada  

A mudança de estado só é confirmada após **estabilidade mínima**, evitando falsos positivos.

### Detecção de Carregamento

O carregamento **não é detectado por nível fixo**, mas por **oscilação elétrica**:

- A0 precisa estar em nível alto (presença no dock);
- A1 deve oscilar entre alto e baixo;
- Após **4 oscilações válidas**, o robô é considerado carregando.

Esse método é significativamente mais confiável do que leitura direta de tensão.

### Detecção de Desligamento da Placa

O robô é considerado desligado quando:

- **A2 > 2000**;
- **A3, A1 e A0 < 900**;
- Estado mantido por **3 ciclos consecutivos**.

Isso evita resets indevidos quando o robô está realmente desligado.


## Saídas Digitais e Acionamento Seguro

### Mapeamento de Saídas

| GPIO ESP32-C3 | Função |
|-------------|-------|
| digitalOutPins[0] | Botão Power (reset do robô) |
| digitalOutPins[1] | Botão de retorno ao carregador |

## Proteção Elétrica Obrigatória

### Acionamento do Botão Power (Reset)

**Não ligar diretamente o botão de power ao ESP32-C3 Mini.**

Para o acionamento do botão de power é **obrigatório** o uso de um **transistor NPN**, como:

- **BC817**

#### Ligação correta:

- **Coletor** → sinal do botão de power do robô  
- **Emissor** → GND  
- **Base** → GPIO do ESP32-C3 (via resistor apropriado)

Essa abordagem:

- Isola eletricamente o ESP32;
- Evita retorno de tensão;
- Impede danos permanentes à GPIO ou ao microcontrolador.


### Acionamento do Botão de Carregamento

Para o botão de retorno ao carregador:

- Um **resistor de 100 Ω em série** é suficiente;
- Pode ser ligado diretamente à porta digital do ESP32-C3;
- Não exige transistor.


## Sequência de Reset Controlado

A sequência executada é totalmente temporizada:

1. Acionamento do botão de power (via transistor);
2. Delay de desligamento;
3. Liberação do botão;
4. Tempo de boot completo;
5. Acionamento do botão de retorno ao carregador;
6. Aguardar estabilização total (~15 s).

Isso garante:

- Reset correto do módulo Wi-Fi;
- Retorno seguro ao dock;
- Nenhuma interferência no LiDAR ou motores.

## Sistema de Segurança

- Watchdog ativo (300 s);
- Bloqueio lógico contra resets em sequência;
- Reset apenas em estado operacional válido;
- Nenhuma escrita em memória do robô;
- Nenhuma interceptação de dados.

## Observações Importantes

- Projeto não oficial;
- Sem vínculo com a Kabum;
- Solução puramente técnica;
- Desenvolvido para resolver um problema real e recorrente.

## Aviso Legal

Este projeto é fornecido **como está**.  
Qualquer modificação física no robô ou na placa é de responsabilidade do usuário.

