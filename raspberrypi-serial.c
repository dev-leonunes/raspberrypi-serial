#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "matriz_leds.h"
#include "hardware/pio.h"
#include "hardware/i2c.h"
#include "inc/ssd1306.h"
#include "inc/ssd1306_font.h"
#include "pico/binary_info.h"
#include <ctype.h>

PIO pio;
uint sm;

const uint LED_GREEN = 11, LED_BLUE = 12, LED_RED = 13;
const uint BTN_A = 5, BTN_B = 6;
#define LED_MATRIX 7
#define I2C_SDA 14
#define I2C_SCL 15

void irq_handler(uint gpio, uint32_t events)
{
    static absolute_time_t last_press = 0;

    // 200ms debounce
    if (absolute_time_diff_us(last_press, get_absolute_time()) > 200000)
    {
        if (gpio == BTN_A)
        {
            bool current_state = gpio_get(LED_GREEN);
            gpio_put(LED_GREEN, !current_state);
            printf("Botão A pressionado! LED Verde %s\n", current_state ? "desligado" : "ligado");
        }
        else if (gpio == BTN_B)
        {
            bool current_state = gpio_get(LED_BLUE);
            gpio_put(LED_BLUE, !current_state);
            printf("Botão B pressionado! LED Azul %s\n", current_state ? "desligado" : "ligado");
        }
        last_press = get_absolute_time();
    }
}

void setup()
{
    // Inicializa a comunicação serial
    stdio_init_all();

    // Configura os pinos dos LEDs como saída
    gpio_init(LED_GREEN);
    gpio_set_dir(LED_GREEN, GPIO_OUT);

    gpio_init(LED_BLUE);
    gpio_set_dir(LED_BLUE, GPIO_OUT);

    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);

    // Configura os pinos dos botões como entrada
    gpio_init(BTN_A);
    gpio_set_dir(BTN_A, GPIO_IN);
    gpio_pull_up(BTN_A);

    gpio_init(BTN_B);
    gpio_set_dir(BTN_B, GPIO_IN);
    gpio_pull_up(BTN_B);

    // Configurar matriz de LEDs
    pio = pio0;
    sm = configurar_matriz(pio);

    // Configurar interrupções para botões (falling edge)
    gpio_set_irq_enabled_with_callback(BTN_A, GPIO_IRQ_EDGE_FALL, true, &irq_handler);
    gpio_set_irq_enabled_with_callback(BTN_B, GPIO_IRQ_EDGE_FALL, true, &irq_handler);

    // Inicialização do i2c
    i2c_init(i2c1, ssd1306_i2c_clock * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Processo de inicialização completo do OLED SSD1306
    ssd1306_init();
}

int main()
{
    setup(); // Configurações iniciais

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

    if (gpio_get(LED_GREEN))
    {
        printf("OOOOI\n");
        ssd1306_draw_char(ssd, 5, 0, 'G');
        render_on_display(ssd, &frame_area);
        sleep_ms(1000);
    }
    else if (gpio_get(LED_BLUE))
    {
        printf("OOOOI\n");
        ssd1306_draw_char(ssd, 5, 0, 'B');
        render_on_display(ssd, &frame_area);
        sleep_ms(1000);
    }

    while (true)
    {
        if (stdio_usb_connected())
        {
            char leitura;
            printf("Digite um caractere: \n");
            scanf(" %c", &leitura);
            if (leitura >= '0' && leitura <= '9')
            {
                exibir_numero(pio, sm, leitura - '0');
                ssd1306_draw_char(ssd, 8, 10, leitura);
                render_on_display(ssd, &frame_area);
                sleep_ms(1000);
            }
            else if (isalpha(leitura))
            {
                limpar_matriz(pio, sm);
                ssd1306_draw_char(ssd, 8, 10, leitura);
                render_on_display(ssd, &frame_area);
                sleep_ms(1000);
            }
            else
            {
                printf("Caractere inválido!\n");
            }
        }
    }

    return 0;
}
