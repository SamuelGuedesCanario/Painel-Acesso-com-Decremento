# Controle de Acesso Interativo

## Descrição do Projeto

Este projeto implementa um painel de controle interativo utilizando a placa BitDogLab com o RP2040 e o sistema operacional de tempo real FreeRTOS. O objetivo é simular o controle de acesso a um espaço físico (como uma sala ou laboratório), permitindo um número limitado de usuários simultâneos.

## Funcionalidades

* Controle de entrada e saída de usuários com botões físicos.
* Capacidade máxima configurável (padrão: 8 usuários).
* Reset do sistema via botão com interrupção.
* Display OLED com informações em tempo real.
* LED RGB indicando o estado do sistema:

  * Azul: 0 usuários
  * Verde: 1 até MAX-2 usuários
  * Amarelo: apenas 2 vagas restantes
  * Vermelho: capacidade máxima atingida
* Buzzer:

  * Beep curto: tentativa de entrada com sistema cheio
  * Beep duplo: reset do sistema

## Recursos Utilizados

* **Placa:** BitDogLab com RP2040
* **FreeRTOS:** Tarefas concorrentes, semáforo de contagem, semáforo binário com interrupção e mutex
* **Display OLED:** I2C com driver SSD1306
* **LED RGB:** GPIO
* **Buzzer:** GPIO com controle de tempo
* **Botões:** GPIO com pull-up interno

## Estrutura do Código

O código principal está dividido em três tarefas:

### 1. vTaskEntrada()

* Verifica o botão de entrada.
* Caso o sistema não esteja cheio, reduz o semáforo de contagem.
* Atualiza o display e o LED RGB.
* Emite beep curto se estiver cheio.

### 2. vTaskSaida()

* Verifica o botão de saída.
* Libera uma vaga no semáforo de contagem.
* Atualiza o display e LED RGB.

### 3. vTaskReset()

* Aguarda semáforo binário sinalizado por interrupção.
* Restaura a contagem de vagas.
* Emite dois beeps curtos.
* Atualiza o display e LED RGB.

## Instruções para Compilação

1. Clone o repositório em sua máquina.
2. Certifique-se de ter o SDK do Pico e FreeRTOS configurados.
3. Compile com `cmake` + `make` ou use uma IDE como VSCode.

---
