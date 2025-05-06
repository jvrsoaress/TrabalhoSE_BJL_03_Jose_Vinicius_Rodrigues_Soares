#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "generated/ws2812.pio.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "FreeRTOS.h"
#include "task.h"

// definições de pinos para os periféricos da placa BitDog Lab (RP2040)
#define I2C_PORT i2c1          // porta I2C usada para o display OLED
#define OLED_ADDRESS 0x3C      // endereço I2C do display OLED
#define BUTTON_A 5             // pino do botão A (para alternar modos)
#define BOTAO_B 6              // pino do botão B (para modo BOOTSEL)
#define WS2812_PIN 7           // pino para a matriz de LEDs WS2812
#define BUZZER 10              // pino do buzzer (para sinais sonoros)
#define LED_G 11               // pino do LED verde do RGB
#define LED_B 12               // pino do LED azul do RGB
#define LED_R 13               // pino do LED vermelho do RGB
#define I2C_SDA 14             // pino SDA para comunicação I2C
#define I2C_SCL 15             // pino SCL para comunicação I2C
#define WIDTH 128              // largura do display OLED 
#define HEIGHT 64              // altura do display OLED 

// enumeração para os estados do semáforo
typedef enum { VERDE, AMARELO, VERMELHO } EstadoSemaforo;

// variáveis globais voláteis para controle do estado e temporização
volatile int modo_noturno = 0;              // flag para alternar entre Modo Normal (0) e Noturno (1)
volatile EstadoSemaforo estado_atual = VERDE; // estado atual do semáforo (começa no Verde)
volatile uint32_t start_time = 0;           // tempo de início de cada estado

// estrutura pra passar o display OLED entre tarefas
typedef struct {
    ssd1306_t disp; // objeto do display OLED
} DisplayData;

// função auxiliar para enviar um pixel à matriz WS2812 (usada para controle de LEDs)
static inline void put_pixel(uint32_t pixel_grb) {
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

// Função auxiliar para converter valores RGB em um formato de 32 bits para a matriz WS2812
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

// configura a matriz de LEDs WS2812 para representar o estado do semáforo
void set_matriz_semaforo(EstadoSemaforo estado) {
    // Mapeamento dos LEDs da matriz 
    int pixel_map[5][5] = {
        {24, 23, 22, 21, 20},
        {15, 16, 17, 18, 19},
        {14, 13, 12, 11, 10},
        {5, 6, 7, 8, 9},
        {4, 3, 2, 1, 0}
    };
    uint32_t pixels[25] = {0}; // array para armazenar os valores RGB dos LEDs

    // define bordas da matriz com brilho baixo para representar a estrutura do semáforo
    for (int i = 0; i < 5; i++) {
        pixels[pixel_map[0][i]] = urgb_u32(1, 1, 1); // linha superior
        pixels[pixel_map[4][i]] = urgb_u32(1, 1, 1); // linha inferior
    }
    for (int i = 1; i < 4; i++) {
        pixels[pixel_map[i][1]] = urgb_u32(1, 1, 1); // coluna esquerda 
        pixels[pixel_map[i][3]] = urgb_u32(1, 1, 1); // coluna direita 
    }

    // zera os LEDs das bordas externas e do centro para destacar as luzes do semáforo
    pixels[pixel_map[0][0]] = 0; pixels[pixel_map[1][0]] = 0; pixels[pixel_map[2][0]] = 0;
    pixels[pixel_map[3][0]] = 0; pixels[pixel_map[4][0]] = 0;
    pixels[pixel_map[0][4]] = 0; pixels[pixel_map[1][4]] = 0; pixels[pixel_map[2][4]] = 0;
    pixels[pixel_map[3][4]] = 0; pixels[pixel_map[4][4]] = 0;
    pixels[pixel_map[1][2]] = 0; pixels[pixel_map[2][2]] = 0; pixels[pixel_map[3][2]] = 0;

    // define a cor do LED central correspondente ao estado do semáforo
    switch (estado) {
        case VERDE: pixels[pixel_map[3][2]] = urgb_u32(0, 32, 0); break;    // Verde na posição inferior
        case AMARELO: pixels[pixel_map[2][2]] = urgb_u32(32, 32, 0); break; // Amarelo na posição central
        case VERMELHO: pixels[pixel_map[1][2]] = urgb_u32(32, 0, 0); break; // Vermelho na posição superior
    }

    for (int i = 0; i < 25; i++) put_pixel(pixels[i]);   // envia os valores RGB para a matriz de LEDs
}

// limpa a matriz de LEDs WS2812, mantendo a estrutura do semáforo visível, mas sem luzes coloridas
void clear_matriz_semaforo() {
    // Mapeamento dos LEDs da matriz 
    int pixel_map[5][5] = {
        {24, 23, 22, 21, 20},
        {15, 16, 17, 18, 19},
        {14, 13, 12, 11, 10},
        {5, 6, 7, 8, 9},
        {4, 3, 2, 1, 0}
    };
    uint32_t pixels[25] = {0}; // array para armazenar os valores RGB dos LEDs

    // define bordas da matriz com brilho baixo para representar a estrutura do semáforo
    for (int i = 0; i < 5; i++) {
        pixels[pixel_map[0][i]] = urgb_u32(1, 1, 1); // linha superior
        pixels[pixel_map[4][i]] = urgb_u32(1, 1, 1); // linha inferior
    }
    for (int i = 1; i < 4; i++) {
        pixels[pixel_map[i][1]] = urgb_u32(1, 1, 1); // coluna esquerda
        pixels[pixel_map[i][3]] = urgb_u32(1, 1, 1); // coluna direita
    }

    // zera os LEDs das bordas externas e do centro
    pixels[pixel_map[0][0]] = 0; pixels[pixel_map[1][0]] = 0; pixels[pixel_map[2][0]] = 0;
    pixels[pixel_map[3][0]] = 0; pixels[pixel_map[4][0]] = 0;
    pixels[pixel_map[0][4]] = 0; pixels[pixel_map[1][4]] = 0; pixels[pixel_map[2][4]] = 0;
    pixels[pixel_map[3][4]] = 0; pixels[pixel_map[4][4]] = 0;
    pixels[pixel_map[1][2]] = 0; pixels[pixel_map[2][2]] = 0; pixels[pixel_map[3][2]] = 0;

    // envia os valores RGB para a matriz de LEDs
    for (int i = 0; i < 25; i++) put_pixel(pixels[i]);
}

// atualiza o display OLED com o estado atual do semáforo
void update_display(ssd1306_t *disp, EstadoSemaforo estado) {
    ssd1306_fill(disp, 0); // limpa o display OLED

    if (modo_noturno) {
        // Modo Noturno
        ssd1306_draw_string(disp, "Modo Noturno", (WIDTH - 12 * 8) / 2, 8);  
        ssd1306_draw_string(disp, "Amarelo", (WIDTH - 7 * 8) / 2, 24);      
        ssd1306_draw_string(disp, "Piscando", (WIDTH - 8 * 8) / 2, 40);      
    } else {
        // Modo Normal
        ssd1306_draw_string(disp, "Modo Normal", (WIDTH - 11 * 8) / 2, 8);  

        switch (estado) {
            case VERDE:
                // estado Verde: exibe "Verde" e "Pode Atravessar"
                ssd1306_draw_string(disp, "Verde", (WIDTH - 5 * 8) / 2, 24);          
                ssd1306_draw_string(disp, "Pode Atravessar", (WIDTH - 14 * 8) / 3, 40); 
                break;
            case AMARELO:
                // estado Amarelo: exibe "Amarelo" e "Atencao"
                ssd1306_draw_string(disp, "Amarelo", (WIDTH - 7 * 8) / 2, 24);  
                ssd1306_draw_string(disp, "Atencao", (WIDTH - 7 * 8) / 2, 40);  
                break;
            case VERMELHO:
                // estado Vermelho: exibe "Vermelho" e "Pare"
                ssd1306_draw_string(disp, "Vermelho", (WIDTH - 8 * 8) / 2, 24); 
                ssd1306_draw_string(disp, "Pare", (WIDTH - 4 * 8) / 2, 40);       
                break;
        }
    }
    ssd1306_send_data(disp); // envia os dados para o display OLED
}

// tarefa que gerencia o ciclo do semáforo e atualiza os periféricos
void vTaskSemaforo(void *pvParameters) {
    DisplayData *data = (DisplayData *)pvParameters; // recebe o display OLED
    start_time = xTaskGetTickCount() * portTICK_PERIOD_MS; // inicializa o tempo de início

    while (1) {
        if (modo_noturno) {
            // Modo Noturno: amarelo piscando (1s ligado, 1s desligado)
            estado_atual = AMARELO;
            gpio_put(LED_R, 1); gpio_put(LED_G, 1); gpio_put(LED_B, 0); // amarelo no LED RGB
            set_matriz_semaforo(estado_atual); // atualiza a matriz de LEDs
            update_display(&data->disp, estado_atual); // atualiza o display OLED
            vTaskDelay(pdMS_TO_TICKS(1000)); // delay de 1s (ligado)

            gpio_put(LED_G, 0); gpio_put(LED_R, 0); // desliga o LED RGB
            clear_matriz_semaforo(); // desativa a luz colorida na matriz
            update_display(&data->disp, estado_atual); // atualiza o display (mantém mensagem)
            vTaskDelay(pdMS_TO_TICKS(1000)); // delay de 1s (desligado)
        } else {
            // modo Normal: ciclo Verde -> Amarelo -> Vermelho
            switch (estado_atual) {
                case VERDE:
                    gpio_put(LED_G, 1); gpio_put(LED_B, 0); gpio_put(LED_R, 0); // verde no LED RGB
                    set_matriz_semaforo(VERDE); // atualiza a matriz de LEDs
                    update_display(&data->disp, VERDE); // atualiza o display OLED
                    vTaskDelay(pdMS_TO_TICKS(5000)); // 5s no Verde
                    estado_atual = AMARELO; // transição para Amarelo
                    start_time = xTaskGetTickCount() * portTICK_PERIOD_MS; // reseta o tempo
                    break;
                case AMARELO:
                    gpio_put(LED_G, 1); gpio_put(LED_B, 0); gpio_put(LED_R, 1); // amarelo no LED RGB
                    set_matriz_semaforo(AMARELO); // atualiza a matriz de LEDs
                    update_display(&data->disp, AMARELO); // atualiza o display OLED
                    vTaskDelay(pdMS_TO_TICKS(2000)); // 2s no Amarelo
                    estado_atual = VERMELHO; // transição para Vermelho
                    start_time = xTaskGetTickCount() * portTICK_PERIOD_MS; // reseta o tempo
                    break;
                case VERMELHO:
                    gpio_put(LED_R, 1); gpio_put(LED_G, 0); gpio_put(LED_B, 0); // vermelho no LED RGB
                    set_matriz_semaforo(VERMELHO); // atualiza a matriz de LEDs
                    update_display(&data->disp, VERMELHO); // atualiza o display OLED
                    vTaskDelay(pdMS_TO_TICKS(5000)); // 5s no Vermelho
                    estado_atual = VERDE; // volta para Verde
                    start_time = xTaskGetTickCount() * portTICK_PERIOD_MS; // reseta o tempo
                    break;
            }
        }
    }
}

// tarefa que gerencia os sinais sonoros do buzzer para acessibilidade
void vTaskBuzzer(void *pvParameters) {
    while (1) {
        uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS; // tempo atual
        uint32_t time_since_start = current_time - start_time; // tempo desde o início do estado
        static int beep_phase = 0; // fase do beep (controla a sequência de sons)
        static bool buzzer_on = false; // estado do buzzer (ligado/desligado)
        static uint32_t last_beep_start = 0; // tempo do último beep
        static EstadoSemaforo last_state = VERDE; // último estado do semáforo

        // reinicia a sequência de beeps ao mudar de estado
        if (last_state != estado_atual) {
            beep_phase = 0;
            buzzer_on = false;
            last_state = estado_atual;
        }

        if (modo_noturno) {
            // Modo Noturno: beep a cada 2s (200ms ligado)
            if (!buzzer_on && time_since_start >= 2000 * beep_phase) {
                gpio_put(BUZZER, 1); // liga o buzzer
                buzzer_on = true;
                last_beep_start = current_time;
                beep_phase++; // avança para a próxima fase
            }
            if (buzzer_on && (current_time - last_beep_start >= 200)) {
                gpio_put(BUZZER, 0); // desliga o buzzer após 200ms
                buzzer_on = false;
            }
        } else {
            // Modo Normal: sons distintos para cada estado
            switch (estado_atual) {
                case VERDE:
                    // Verde: 1 beep curto (200ms) após 2.5s
                    if (!buzzer_on && time_since_start >= 2500 && beep_phase == 0) {
                        gpio_put(BUZZER, 1);
                        buzzer_on = true;
                        last_beep_start = current_time;
                        beep_phase++;
                    }
                    if (buzzer_on && (current_time - last_beep_start >= 200)) {
                        gpio_put(BUZZER, 0);
                        buzzer_on = false;
                    }
                    break;
                case AMARELO:
                    // Amarelo: 4 beeps rápidos (100ms cada) a cada 200ms
                    if (beep_phase < 4) {
                        if (!buzzer_on && time_since_start >= 200 * beep_phase) {
                            gpio_put(BUZZER, 1);
                            buzzer_on = true;
                            last_beep_start = current_time;
                        }
                        if (buzzer_on && (current_time - last_beep_start >= 100)) {
                            gpio_put(BUZZER, 0);
                            buzzer_on = false;
                            beep_phase++;
                        }
                    }
                    break;
                case VERMELHO:
                    // Vermelho: 2 beeps (200ms) com 1.5s de intervalo (a cada 2s)
                    if (beep_phase < 2) {
                        if (!buzzer_on && time_since_start >= 2000 * (beep_phase + 1)) {
                            gpio_put(BUZZER, 1);
                            buzzer_on = true;
                            last_beep_start = current_time;
                            beep_phase++;
                        }
                        if (buzzer_on && (current_time - last_beep_start >= 200)) {
                            gpio_put(BUZZER, 0);
                            buzzer_on = false;
                        }
                    }
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // delay para evitar sobrecarga na tarefa
    }
}

// tarefa que monitora o botão A e alterna entre os modos
void vTaskBotao(void *pvParameters) {
    TickType_t last_wake_time = xTaskGetTickCount(); // controle de tempo para delay periódico
    bool button_pressed = false; // estado do botão p/ evita múltiplas detecções

    while (1) {
        bool current_state = !gpio_get(BUTTON_A); // botão pressionado = 0 (pull-up ativo)

        // detecta transição de não pressionado para pressionado
        if (current_state && !button_pressed) {
            modo_noturno = !modo_noturno; // alterna o modo
            estado_atual = (modo_noturno == 0) ? VERDE : AMARELO; // define o estado inicial do modo
            start_time = xTaskGetTickCount() * portTICK_PERIOD_MS; // reseta o tempo
            button_pressed = true; // marca o botão como pressionado
            vTaskDelay(pdMS_TO_TICKS(200)); // debounce de 200ms para evitar bounces
        } else if (!current_state) {
            button_pressed = false; // reseta ao soltar o botão
        }

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(10)); // delay periódico de 10ms
    }
}

// tarefa auxiliar para monitoramento 
void vTaskDisplay(void *pvParameters) {
    DisplayData *data = (DisplayData *)pvParameters; // recebe o display OLED
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(50)); // delay de 50ms (apenas monitora)
    }
}

// função de interrupção para o modo BOOTSEL ao pressionar o botão B
#include "pico/bootrom.h"
void gpio_irq_handler(uint gpio, uint32_t events) {
    if (gpio == BOTAO_B && events == GPIO_IRQ_EDGE_FALL) { 
        reset_usb_boot(0, 0); // entra no modo BOOTSEL para atualização via USB
    }
}

// função principal: inicializa os periféricos e cria as tarefas
int main() {
    stdio_init_all(); 
    sleep_ms(2000); // aguarda 2s para estabilizar a inicialização

    // inicialização do I2C e OLED
    i2c_init(I2C_PORT, 400 * 1000); // configura I2C a 400kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // define SDA como função I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // define SCL como função I2C
    gpio_pull_up(I2C_SDA); // ativa pull-up no SDA
    gpio_pull_up(I2C_SCL); // ativa pull-up no SCL
    sleep_ms(500); // aguarda 500ms para estabilizar o I2C

    ssd1306_t disp; // estrutura para o display OLED
    disp.external_vcc = false; // sem alimentação externa
    ssd1306_init(&disp, WIDTH, HEIGHT, false, OLED_ADDRESS, I2C_PORT); // inicializa o display
    ssd1306_config(&disp); // configura o display
    ssd1306_fill(&disp, 0); // limpa o display
    ssd1306_send_data(&disp); // envia os dados para o display

    // inicialização dos GPIOs
    gpio_init(BUTTON_A); gpio_set_dir(BUTTON_A, GPIO_IN); gpio_pull_up(BUTTON_A); // configura botão A como entrada com pull-up
    gpio_init(BUZZER); gpio_set_dir(BUZZER, GPIO_OUT); // configura buzzer como saída
    gpio_init(LED_G); gpio_set_dir(LED_G, GPIO_OUT); // configura LED verde como saída
    gpio_init(LED_B); gpio_set_dir(LED_B, GPIO_OUT); // configura LED azul como saída
    gpio_init(LED_R); gpio_set_dir(LED_R, GPIO_OUT); // configura LED vermelho como saída

    // inicialização da matriz de LEDs WS2812
    PIO pio = pio0; // usa PIO0 para comunicação com WS2812
    uint offset = pio_add_program(pio, &ws2812_program); // carrega o programa WS2812 no PIO
    ws2812_program_init(pio, 0, offset, WS2812_PIN, 800000, false); // inicializa WS2812 a 800kHz

    // Configuração do botão B para modo BOOTSEL
    gpio_init(BOTAO_B);
    gpio_set_dir(BOTAO_B, GPIO_IN);
    gpio_pull_up(BOTAO_B);
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // passagem de dados do display para as tarefas
    DisplayData data = {disp};

    // criação das tarefas do FreeRTOS
    xTaskCreate(vTaskSemaforo, "Semaforo", 256, &data, tskIDLE_PRIORITY + 1, NULL); // tarefa para o semáforo
    xTaskCreate(vTaskBuzzer, "Buzzer", 256, NULL, tskIDLE_PRIORITY + 1, NULL); // tarefa para o buzzer
    xTaskCreate(vTaskBotao, "Botao", 256, NULL, tskIDLE_PRIORITY + 1, NULL); // tarefa para o botão
    xTaskCreate(vTaskDisplay, "Display", 256, &data, tskIDLE_PRIORITY + 1, NULL); // tarefa para monitoramento

    vTaskStartScheduler(); // inicia o escalonador do FreeRTOS

    while (1); 
}