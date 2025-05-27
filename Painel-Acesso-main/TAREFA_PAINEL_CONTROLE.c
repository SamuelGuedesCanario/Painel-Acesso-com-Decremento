#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "lib/ssd1306.h"
#include "lib/buzzer.h"

#define MAX_USUARIOS     8
#define PINO_ENTRADA     5   // Botão A
#define PINO_SAIDA       6   // Botão B
#define PINO_RESET       22  // Botão do joystick
#define PINO_BUZZER      21
#define LED_R            13
#define LED_G            11
#define LED_B            12

#define I2C_PORT         i2c1
#define I2C_SDA          14
#define I2C_SCL          15
#define DISPLAY_ADDR     0x3C

//VARIÁVEIS GLOBAIS
SemaphoreHandle_t xSemUsuarios;
SemaphoreHandle_t xSemReset;
SemaphoreHandle_t xMutexDisplay;

ssd1306_t ssd;

//FUNÇÕES AUXILIARES
void atualizar_display(uint count) {
    xSemaphoreTake(xMutexDisplay, portMAX_DELAY);

    ssd1306_fill(&ssd, false);
    ssd1306_rect(&ssd, 0, 0, 127, 63, true, false);
    ssd1306_draw_string(&ssd, "PAINEL ACESSO", 12, 3);
    char msg[32];
    sprintf(msg, "Usuarios: %d/%d", count, MAX_USUARIOS);
    ssd1306_draw_string(&ssd, msg, 5, 40);
    ssd1306_send_data(&ssd);

    xSemaphoreGive(xMutexDisplay);
}

void atualizar_led(uint count) {
    if (count == 0) {
        gpio_put(LED_R, 0);
        gpio_put(LED_G, 0);
        gpio_put(LED_B, 1); // Azul
    } else if (count < MAX_USUARIOS - 1) {
        gpio_put(LED_R, 0);
        gpio_put(LED_G, 1);
        gpio_put(LED_B, 0); // Verde
    } else if (count == MAX_USUARIOS - 1) {
        gpio_put(LED_R, 1);
        gpio_put(LED_G, 1);
        gpio_put(LED_B, 0); // Amarelo
    } else {
        gpio_put(LED_R, 1);
        gpio_put(LED_G, 0);
        gpio_put(LED_B, 0); // Vermelho
    }
}

//TAREFAS
void vTaskEntrada(void *params) {
    uint count;
    while (1) {
        if (gpio_get(PINO_ENTRADA) == 0) {
            if (xSemaphoreTake(xSemUsuarios, 0) == pdTRUE) {
                count = MAX_USUARIOS - uxSemaphoreGetCount(xSemUsuarios);
                atualizar_display(count);
                atualizar_led(count);
            } else {
                buzzer_play_tone(440, 100); // Sistema cheio
            }
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void vTaskSaida(void *params) {
    uint count;
    while (1) {
        if (gpio_get(PINO_SAIDA) == 0) {
            if (uxSemaphoreGetCount(xSemUsuarios) < MAX_USUARIOS) {
                xSemaphoreGive(xSemUsuarios);
                count = MAX_USUARIOS - uxSemaphoreGetCount(xSemUsuarios);
                atualizar_display(count);
                atualizar_led(count);
            }
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

void vTaskReset(void *params) {
    uint count;
    while (1) {
        if (xSemaphoreTake(xSemReset, portMAX_DELAY)) {
            // Recria semáforo de contagem para resetar
            vSemaphoreDelete(xSemUsuarios);
            xSemUsuarios = xSemaphoreCreateCounting(MAX_USUARIOS, MAX_USUARIOS);

            buzzer_play_tone(440, 100);
            vTaskDelay(pdMS_TO_TICKS(200));
            buzzer_play_tone(440, 100);

            count = 0;
            atualizar_display(count);
            atualizar_led(count);
        }
    }
}

void gpio_irq_handler(uint gpio, uint32_t events) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xSemaphoreGiveFromISR(xSemReset, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

int main() {
    stdio_init_all();

    // GPIOs
    gpio_init(PINO_ENTRADA);
    gpio_set_dir(PINO_ENTRADA, GPIO_IN);
    gpio_pull_up(PINO_ENTRADA);

    gpio_init(PINO_SAIDA);
    gpio_set_dir(PINO_SAIDA, GPIO_IN);
    gpio_pull_up(PINO_SAIDA);

    gpio_init(PINO_RESET);
    gpio_set_dir(PINO_RESET, GPIO_IN);
    gpio_pull_up(PINO_RESET);
    gpio_set_irq_enabled_with_callback(PINO_RESET, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    gpio_init(LED_R);
    gpio_init(LED_G);
    gpio_init(LED_B);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_set_dir(LED_B, GPIO_OUT);

    buzzer_init();

    i2c_init(I2C_PORT, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&ssd, 128, 64, false, DISPLAY_ADDR, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    // Semáforos
    xSemUsuarios = xSemaphoreCreateCounting(MAX_USUARIOS, MAX_USUARIOS);
    xSemReset = xSemaphoreCreateBinary();
    xMutexDisplay = xSemaphoreCreateMutex();

    // Tarefas
    xTaskCreate(vTaskEntrada, "Entrada", 256, NULL, 1, NULL);
    xTaskCreate(vTaskSaida, "Saida", 256, NULL, 1, NULL);
    xTaskCreate(vTaskReset, "Reset", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while (1) {}
}
