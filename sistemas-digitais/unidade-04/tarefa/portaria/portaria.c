#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "inc/ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"

int inputs[] = {0, 0, 0, 0};

// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2818b.pio.h"

// Definição do número de LEDs e pino.
#define LED_COUNT 25
#define LED_PIN 7

// Definição de pixel GRB
struct pixel_t {
  uint8_t G, R, B; // Três valores de 8-bits compõem um pixel.
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; // Mudança de nome de "struct pixel_t" para "npLED_t" por clareza.

// Declaração do buffer de pixels que formam a matriz.
npLED_t leds[LED_COUNT];

// Variáveis para uso da máquina PIO.
PIO np_pio;
uint sm;

/**
 * Inicializa a máquina PIO para controle da matriz de LEDs.
 */
void npInit(uint pin) {

  // Cria programa PIO.
  uint offset = pio_add_program(pio0, &ws2818b_program);
  np_pio = pio0;

  // Toma posse de uma máquina PIO.
  sm = pio_claim_unused_sm(np_pio, false);
  if (sm < 0) {
    np_pio = pio1;
    sm = pio_claim_unused_sm(np_pio, true); // Se nenhuma máquina estiver livre, panic!
  }

  // Inicia programa na máquina PIO obtida.
  ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

  // Limpa buffer de pixels.
  for (uint i = 0; i < LED_COUNT; ++i) {
    leds[i].R = 0;
    leds[i].G = 0;
    leds[i].B = 0;
  }
}

/**
 * Atribui uma cor RGB a um LED.
 */
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
  leds[index].R = r;
  leds[index].G = g;
  leds[index].B = b;
}

/**
 * Limpa o buffer de pixels.
 */
void npClear() {
  for (uint i = 0; i < LED_COUNT; ++i)
    npSetLED(i, 0, 0, 0);
}

/**
 * Escreve os dados do buffer nos LEDs.
 */
void npWrite() {
  // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
  for (uint i = 0; i < LED_COUNT; ++i) {
    pio_sm_put_blocking(np_pio, sm, leds[i].G);
    pio_sm_put_blocking(np_pio, sm, leds[i].R);
    pio_sm_put_blocking(np_pio, sm, leds[i].B);
  }
  sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}

// Leds
#define LED_R_PIN 13
#define LED_G_PIN 11

// Botoes
#define BTN_A_PIN 5
#define BTN_B_PIN 6
const int SW = 22;

const uint I2C_SDA = 14;
const uint I2C_SCL = 15;

// Assinaturas de todas as funcoes
void limpar();
void escrever_msg(char *msg);

// Função que inicializa todos os pinos
void setup() {
    stdio_init_all();   // Inicializa os tipos stdio padrão presentes ligados ao binário

    // Inicializa matriz de LEDs NeoPixel.
    npInit(LED_PIN);
    npClear();

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

    gpio_init(SW);
    gpio_set_dir(SW, GPIO_IN);
    gpio_pull_up(SW);
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
            estado = (estado + 1) % 4;
        } else if (adc_y_raw < 1400) {
            // Retrocede para o estado anterior (ciclo circular)
            estado = (estado + 3) % 4;  // Adiciona 6 para evitar números negativos
        }

        printf("%u\n", adc_y_raw);
        printf("%u\n\n", estado);

        switch (estado)
        {
            case 0:
                gr();
                break;
            case 1:
                ho();
                break;
            case 2:
                di();
                break;
            case 3:
                pt();
                break;
        }


        atualizar_inputs();
        atualizar_catraca_led();

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
void escrever_msg(char *msg) {
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
        msg,
        "                ",
        "                ",
        "                ",
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

void atualizar_inputs() {
    npClear();

    if (inputs[0] == 1) {
        npSetLED(4, 0, 255, 0);
    } else {
        npSetLED(4, 255, 0, 0);
    }

    if (inputs[1] == 1) {
        npSetLED(3, 0, 255, 0);
    } else {
        npSetLED(3, 255, 0, 0);
    }

    if (inputs[2] == 1) {
        npSetLED(2, 0, 255, 0);
    } else {
        npSetLED(2, 255, 0, 0);
    }

    if (inputs[3] == 1) {
        npSetLED(1, 0, 255, 0);
    } else {
        npSetLED(1, 255, 0, 0);
    }

    npWrite();
}

void atualizar_catraca_led() {
    int resultado = (inputs[0] && inputs[1] && inputs[2]) || !inputs[3];

    if (resultado) {
        gpio_put(LED_G_PIN, 1);
        gpio_put(LED_R_PIN, 0);
    } else {
        gpio_put(LED_G_PIN, 0);
        gpio_put(LED_R_PIN, 1);
    }

    npWrite();
}


void joystick_pressionado(indice) {
    if (gpio_get(SW) == 0) {
        printf("Botao pressionado\n");
        printf("Indice: %d\n", indice);
        printf("Valor: %d\n", inputs[indice]);

        if (inputs[indice] == 1) {
            inputs[indice] = 0;
        } else {
            inputs[indice] = 1;
        }
    }
}

void gr() {
    escrever_msg("0 - GR");
    joystick_pressionado(0);
}

void ho() {
    escrever_msg("1 - HO");
    joystick_pressionado(1);
}

void di() {
    escrever_msg("2 - DI");
    joystick_pressionado(2);
}

void pt() {
    escrever_msg("3 - PT");
    joystick_pressionado(3);
}