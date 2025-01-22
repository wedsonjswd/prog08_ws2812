#include <stdio.h>                  // Biblioteca padrão de entrada e saída
#include "pico/stdlib.h"            // Biblioteca padrão para o Raspberry Pi Pico
#include <hardware/pio.h>           // Biblioteca para manipulação de periféricos PIO
#include "hardware/clocks.h"        // Biblioteca para controle de relógios do hardware
#include "ws2818b.pio.h"            // Biblioteca PIO para controle de LEDs WS2818B
#include "hardware/adc.h"           // Biblioteca para controle do ADC (Conversor Analógico-Digital)

// Definições de constantes
#define LED_COUNT 25                // Número de LEDs na matriz
#define LED_PIN 7                   // Pino GPIO conectado aos LEDs

#define JOYSTICK_X 27               // Pino GPIO para o eixo X do joystick
#define JOYSTICK_Y 26               // Pino GPIO para o eixo Y do joystick

// Estrutura para representar um pixel com componentes RGB
struct pixel_t { 
    uint8_t G, R, B;                // Componentes de cor: Verde, Vermelho e Azul
};

typedef struct pixel_t pixel_t;     // Alias para a estrutura pixel_t
typedef pixel_t npLED_t;            // Alias para facilitar o uso no contexto de LEDs

npLED_t leds[LED_COUNT];            // Array para armazenar o estado de cada LED
PIO np_pio;                         // Variável para referenciar a instância PIO usada
uint sm;                            // Variável para armazenar o número do state machine usado

// Função para inicializar o PIO para controle dos LEDs
void npInit(uint pin) 
{
    uint offset = pio_add_program(pio0, &ws2818b_program); // Carregar o programa PIO
    np_pio = pio0;                                         // Usar o primeiro bloco PIO

    sm = pio_claim_unused_sm(np_pio, false);              // Tentar usar uma state machine do pio0
    if (sm < 0)                                           // Se não houver disponível no pio0
    {
        np_pio = pio1;                                    // Mudar para o pio1
        sm = pio_claim_unused_sm(np_pio, true);           // Usar uma state machine do pio1
    }

    ws2818b_program_init(np_pio, sm, offset, pin, 800000.f); // Inicializar state machine para LEDs

    for (uint i = 0; i < LED_COUNT; ++i)                  // Inicializar todos os LEDs como apagados
    {
        leds[i].R = 0;
        leds[i].G = 0;
        leds[i].B = 0;
    }
}

// Função para definir a cor de um LED específico
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) 
{
    leds[index].R = r;                                    // Definir componente vermelho
    leds[index].G = g;                                    // Definir componente verde
    leds[index].B = b;                                    // Definir componente azul
}

// Função para limpar (apagar) todos os LEDs
void npClear() 
{
    for (uint i = 0; i < LED_COUNT; ++i)                  // Iterar sobre todos os LEDs
        npSetLED(i, 0, 0, 0);                             // Definir cor como preta (apagado)
}

// Função para atualizar os LEDs no hardware
void npWrite() 
{
    for (uint i = 0; i < LED_COUNT; ++i)                  // Iterar sobre todos os LEDs
    {
        pio_sm_put_blocking(np_pio, sm, leds[i].G);       // Enviar componente verde
        pio_sm_put_blocking(np_pio, sm, leds[i].R);       // Enviar componente vermelho
        pio_sm_put_blocking(np_pio, sm, leds[i].B);       // Enviar componente azul
    }
}

// Função para inicializar o joystick
void joystickInit() 
{
    adc_init();                                           // Inicializar o ADC
    adc_gpio_init(JOYSTICK_X);                            // Configurar GPIO para eixo X
    adc_gpio_init(JOYSTICK_Y);                            // Configurar GPIO para eixo Y
}

// Função para obter a posição do joystick e mapear para o índice do LED
int getJoystickLEDIndex() 
{
    adc_select_input(1);                                  // Selecionar o ADC para o eixo Y
    uint16_t y_value = adc_read();                        // Ler valor do eixo Y

    adc_select_input(0);                                  // Selecionar o ADC para o eixo X
    uint16_t x_value = adc_read();                        // Ler valor do eixo X

    // Mapear valores do joystick (0-4095) para os índices da matriz (0-24)
    int row = x_value * 5 / 4096;                         // Mapear eixo X para as linhas
    int col = (4096 - y_value) * 5 / 4096;                // Mapear eixo Y para as colunas (invertido)

    // Garantir que os índices estejam dentro dos limites
    if (row >= 5) row = 4;                                // Limitar valor máximo das linhas
    if (col >= 5) col = 4;                                // Limitar valor máximo das colunas

    return row * 5 + col;                                 // Calcular índice baseado em linha e coluna
}

int main() 
{
    stdio_init_all();                                     // Inicializar a comunicação serial
    npInit(LED_PIN);                                      // Inicializar os LEDs
    npClear();                                            // Apagar todos os LEDs
    npWrite();                                            // Atualizar o estado inicial dos LEDs

    joystickInit();                                       // Inicializar o joystick

    while (true)                                          // Loop principal
    {
        int ledIndex = getJoystickLEDIndex();             // Obter o índice do LED baseado no joystick

        npClear();                                        // Limpar todos os LEDs
        npSetLED(ledIndex, 0, 255, 0);                    // Acender o LED correspondente (verde)
        npWrite();                                        // Atualizar LEDs

        printf("Joystick LED Index: %d\n", ledIndex);     // Exibir posição do LED na serial

        sleep_ms(100);                                    // Pequeno atraso para estabilidade
    }
}