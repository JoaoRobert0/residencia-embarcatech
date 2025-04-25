#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

void limpar() {
    // Preparar área de renderização para o display (ssd1306_width pixels por ssd1306_n_pages páginas)
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

int main()
{
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
                escrever_porta_logica("AND");
                break;
            case 1:
                escrever_porta_logica("OR");
                break;
            case 2:
                escrever_porta_logica("NOT");
                break;
            case 3:
                escrever_porta_logica("NAND");
                break;
            case 4:
                escrever_porta_logica("NOR");
                break;
            case 5:
                escrever_porta_logica("XOR");
                break;
            case 6:
                escrever_porta_logica("XNOR");
                break;
        }
        sleep_ms(200);
    }

    return 0;
}