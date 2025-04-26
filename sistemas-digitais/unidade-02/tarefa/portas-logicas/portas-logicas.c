#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"

// Leds
#define LED_R_PIN 13
#define LED_G_PIN 11

// Botoes
#define BTN_A_PIN 5
#define BTN_B_PIN 6

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

// Assinaturas de todas as funcoes
void limpar();
void escrever_porta_logica(char *msg);
void and();
void or();
void not();
void nand();
void nor();
void xor();
void xnor();

// Função que inicializa todos os pinos
void setup() {
    stdio_init_all();   // Inicializa os tipos stdio padrão presentes ligados ao binário

    // Inicialização do i2c
    i2c_init(i2c1, 700 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Processo de inicialização completo do OLED SSD1306
    ssd1306_init();

    // Inicializa o conversor analógico-digital (ADC)
    adc_init();

    // Configura os pinos GPIO 26 e 27 como entradas de ADC (alta impedância, sem resistores pull-up)
    adc_gpio_init(26);
    adc_gpio_init(27);

    gpio_init(LED_R_PIN);
    gpio_set_dir(LED_R_PIN, GPIO_OUT);

    gpio_init(LED_G_PIN);
    gpio_set_dir(LED_G_PIN, GPIO_OUT);

    gpio_init(BTN_A_PIN);
    gpio_set_dir(BTN_A_PIN, GPIO_IN);
    gpio_pull_up(BTN_A_PIN);

    gpio_init(BTN_B_PIN);
    gpio_set_dir(BTN_B_PIN, GPIO_IN);
    gpio_pull_up(BTN_B_PIN);
}

int main()
{    
    // Realizar o setup
    setup();

    uint8_t estado = 0;
    while (1) {
        // Seleciona a entrada ADC 1 (conectada ao eixo Y do joystick)
        adc_select_input(0);
        // Lê o valor do ADC para o eixo Y
        uint adc_y_raw = adc_read();

        if (adc_y_raw > 3000) {
            // Avança para o próximo estado (ciclo circular)
            estado = (estado + 1) % 7;
        } else if (adc_y_raw < 1400) {
            // Retrocede para o estado anterior (ciclo circular)
            estado = (estado + 6) % 7;  // Adiciona 6 para evitar números negativos
        }

        printf("%u\n", adc_y_raw);
        printf("%u\n\n", estado);

        switch (estado) {
            case 0:
                and();
                break;
            case 1:
                or();
                break;
            case 2:
                not();
                break;
            case 3:
                nand();
                break;
            case 4:
                nor();
                break;
            case 5:
                xor();
                break;
            case 6:
                xnor();
                break;
        }
        sleep_ms(200);
    }

    return 0;
}

// Função para limpar display o-led
void limpar() {
    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    // zera o display inteiro
    uint8_t ssd[ssd1306_buffer_length];
    memset(ssd, 0, ssd1306_buffer_length);
    render_on_display(ssd, &frame_area);
}

// Função para escrever o nome da porta logica no display o-led
void escrever_porta_logica(char *msg) {
    // Limpar o display antes de usar
    limpar();

    struct render_area frame_area = {
        start_column : 0,
        end_column : ssd1306_width - 1,
        start_page : 0,
        end_page : ssd1306_n_pages - 1
    };

    calculate_render_area_buffer_length(&frame_area);

    uint8_t ssd[ssd1306_buffer_length];

    char *text[] = {
        "                ",
        "                ",
        "                ",
        msg,
        "                ",
    };

    int y = 0;
    for (uint i = 0; i < count_of(text); i++)
    {
        ssd1306_draw_string(ssd, 5, y, text[i]);
        y += 8;
    }
    render_on_display(ssd, &frame_area);
}

// Função que implementa a logica da porta and
void and() {
    escrever_porta_logica("      AND");

    if (gpio_get(BTN_A_PIN) == 1 && gpio_get(BTN_B_PIN) == 1) {
        gpio_put(LED_G_PIN, 1);
        gpio_put(LED_R_PIN, 0);
    } else {
        gpio_put(LED_R_PIN, 1);
        gpio_put(LED_G_PIN, 0);
    }
}

// Função que implementa a logica da porta or
void or() {
    escrever_porta_logica("      OR");

    if (gpio_get(BTN_A_PIN) == 1 || gpio_get(BTN_B_PIN) == 1) {
        gpio_put(LED_G_PIN, 1);
        gpio_put(LED_R_PIN, 0);
    } else {
        gpio_put(LED_R_PIN, 1);
        gpio_put(LED_G_PIN, 0);
    }
}

// Função que implementa a logica da porta not
void not() {
    escrever_porta_logica("      NOT");

    if (gpio_get(BTN_A_PIN) == 1) {
        gpio_put(LED_G_PIN, 0);
        gpio_put(LED_R_PIN, 1);
    } else {
        gpio_put(LED_R_PIN, 0);
        gpio_put(LED_G_PIN, 1);
    }
}

// Função que implementa a logica da porta nand
void nand() {
    escrever_porta_logica("      NAND");

    if (gpio_get(BTN_A_PIN) == 1 && gpio_get(BTN_B_PIN) == 1) {
        gpio_put(LED_G_PIN, 0);
        gpio_put(LED_R_PIN, 1);
    } else {
        gpio_put(LED_R_PIN, 0);
        gpio_put(LED_G_PIN, 1);
    }
}

// Função que implementa a logica da porta nor
void nor() {
    escrever_porta_logica("      NOR");

    if (gpio_get(BTN_A_PIN) == 1 || gpio_get(BTN_B_PIN) == 1) {
        gpio_put(LED_G_PIN, 0);
        gpio_put(LED_R_PIN, 1);
    } else {
        gpio_put(LED_R_PIN, 0);
        gpio_put(LED_G_PIN, 1);
    }
}

// Função que implementa a logica da porta xor
void xor() {
    escrever_porta_logica("      XOR");

    if ((gpio_get(BTN_A_PIN) == 1 && gpio_get(BTN_B_PIN) == 1) || 
        (gpio_get(BTN_A_PIN) == 0 && gpio_get(BTN_B_PIN) == 0)) 
    {
        gpio_put(LED_G_PIN, 0);
        gpio_put(LED_R_PIN, 1);
    } else {
        gpio_put(LED_R_PIN, 0);
        gpio_put(LED_G_PIN, 1);
    }
}

// Função que implementa a logica da porta xnor
void xnor() {
    escrever_porta_logica("      XNOR");

    if ((gpio_get(BTN_A_PIN) == 1 && gpio_get(BTN_B_PIN) == 1) || 
        (gpio_get(BTN_A_PIN) == 0 && gpio_get(BTN_B_PIN) == 0)) 
    {
        gpio_put(LED_G_PIN, 1);
        gpio_put(LED_R_PIN, 0);
    } else {
        gpio_put(LED_R_PIN, 1);
        gpio_put(LED_G_PIN, 0);
    }
}