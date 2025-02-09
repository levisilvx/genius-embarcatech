#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

// Biblioteca gerada pelo arquivo .pio durante compilação.
#include "ws2818b.pio.h"

// Definição do número de LEDs e pino.
#define LED_COUNT 25
#define LED_PIN 7

//Definição dos Botões A e B
#define BTN_PIN 5
#define BTN_PIN2 6

//Definição dos Buzzers
#define BUZZER_PIN1 10
#define BUZZER_PIN2 21

//Definições para delay de led e de debounce
#define LED_DISPLAY_TIME 500
#define BUTTON_DEBOUNCE_TIME 100

//Definição para o tamanho máximo da sequência do jogo
#define MAX_SEQUENCE_LENGTH 100

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

// Variáveis globais
uint8_t sequence[MAX_SEQUENCE_LENGTH]; //Array para armazenar a sequência
uint16_t sequenceLength = 0;           //Tamanho da sequencia em cada rodada do jogo
uint16_t totalScore = 0;               //Pontuação total 
uint16_t currentRoundScore = 0;        //Pontuação de cada rodada do jogo

//Inicializa a máquina PIO para controle da matriz de LEDs.
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

//Atribui uma cor RGB a um LED.
void npSetLED(const uint index, const uint8_t r, const uint8_t g, const uint8_t b) {
  leds[index].R = r;
  leds[index].G = g;
  leds[index].B = b;
}

//Limpa o buffer de pixels.
void npClear() {
  for (uint i = 0; i < LED_COUNT; ++i)
    npSetLED(i, 0, 0, 0);
}

//Escreve os dados do buffer nos LEDs.
void npWrite() {
  // Escreve cada dado de 8-bits dos pixels em sequência no buffer da máquina PIO.
  for (uint i = 0; i < LED_COUNT; ++i) {
    pio_sm_put_blocking(np_pio, sm, leds[i].G);
    pio_sm_put_blocking(np_pio, sm, leds[i].R);
    pio_sm_put_blocking(np_pio, sm, leds[i].B);
  }
  sleep_us(100); // Espera 100us, sinal de RESET do datasheet.
}

//Inicializa os Botões A e B.
void btnInit(){
  gpio_init(BTN_PIN);
  gpio_set_dir(BTN_PIN, GPIO_IN);
  gpio_pull_up(BTN_PIN);

  gpio_init(BTN_PIN2);
  gpio_set_dir(BTN_PIN2, GPIO_IN);
  gpio_pull_up(BTN_PIN2);
}

//Inicializa os buzzers.
void buzzerInit() {
  gpio_init(BUZZER_PIN1);
  gpio_set_dir(BUZZER_PIN1, GPIO_OUT);

  gpio_init(BUZZER_PIN2);
  gpio_set_dir(BUZZER_PIN2, GPIO_OUT);
}

//Gera um som para a cor amarela.
void playYellowSound() {
  for (int i = 0; i < 200; i++) {
    gpio_put(BUZZER_PIN1, 1);
    sleep_us(500);
    gpio_put(BUZZER_PIN1, 0);
    sleep_us(500);
  }
}

//Gera um som para a cor azul.
void playBlueSound() {
  for (int i = 0; i < 200; i++) {
    gpio_put(BUZZER_PIN2, 1);
    sleep_us(400);
    gpio_put(BUZZER_PIN2, 0);
    sleep_us(400);
  }
}

//Gera um som para erro.
void playErrorSound() {
  for (int i = 0; i < 100; i++) {
    gpio_put(BUZZER_PIN1, 1);
    sleep_us(1000);
    gpio_put(BUZZER_PIN1, 0);
    sleep_us(1000);
  }
}

//Gera um som para acerto.
void playSuccessSound() {
  for (int i = 0; i < 200; i++) {
    gpio_put(BUZZER_PIN2, 1);
    sleep_us(250);
    gpio_put(BUZZER_PIN2, 0);
    sleep_us(250);
  }
}

//Seta o buffer da matriz de LEDs para o "X" de quando o jogador perde.
void npSetBufferRed(){
  npSetLED(4, 90, 0, 0);   // Define o LED de índice 4 para vermelho.
  npSetLED(6, 90, 0, 0);   // Define o LED de índice 6 para vermelho.
  npSetLED(12, 90, 0, 0);  // Define o LED de índice 12 para vermelho.
  npSetLED(18, 90, 0, 0);  // Define o LED de índice 18 para vermelho.
  npSetLED(20, 90, 0, 0);  // Define o LED de índice 20 para vermelho.
  npSetLED(24, 90, 0, 0);  // Define o LED de índice 24 para vermelho.
  npSetLED(8, 90, 0, 0);   // Define o LED de índice 8 para vermelho.
  npSetLED(16, 90, 0, 0);  // Define o LED de índice 16 para vermelho.
  npSetLED(0, 90, 0, 0);   // Define o LED de índice 0 para vermelho.
}

//Seta o buffer da matriz de LEDs para o indicador AMARELO.
void npSetBufferYellow(){
  npSetLED(2, 90, 200, 0);   // Define o LED de índice 2 para amarelo.
  npSetLED(7, 90, 200, 0);   // Define o LED de índice 7 para amarelo.
  npSetLED(12, 90, 200, 0);  // Define o LED de índice 12 para amarelo.
  npSetLED(17, 90, 200, 0);  // Define o LED de índice 17 para amarelo.
  npSetLED(22, 90, 200, 0);  // Define o LED de índice 22 para amarelo.

  npSetLED(6, 90, 200, 0);   // Define o LED de índice 6 para amarelo.
  npSetLED(13, 90, 200, 0);  // Define o LED de índice 13 para amarelo.
  npSetLED(16, 90, 200, 0);  // Define o LED de índice 16 para amarelo.

  npSetLED(14, 90, 200, 0);  // Define o LED de índice 14 para amarelo.
}

//Seta o buffer da matriz de LEDs para um "Check".
void npSetBufferGreen(){
  npSetLED(13, 200, 90, 0);  // Define o LED de índice 13 para verde.
  npSetLED(7, 200, 90, 0);   // Define o LED de índice 7 para verde.
  npSetLED(11, 200, 90, 0);  // Define o LED de índice 11 para verde.
  npSetLED(19, 200, 90, 0);  // Define o LED de índice 19 para verde.
}

//Seta o buffer da matriz de LEDs para o indicador AZUL.
void npSetBufferBlue(){
  npSetLED(2, 0, 0, 220);   // Define o LED de índice 2 para azul.
  npSetLED(7, 0, 0, 220);   // Define o LED de índice 7 para azul.
  npSetLED(12, 0, 0, 220);  // Define o LED de índice 12 para azul.
  npSetLED(17, 0, 0, 220);  // Define o LED de índice 17 para azul.
  npSetLED(22, 0, 0, 220);  // Define o LED de índice 22 para azul.

  npSetLED(8, 0, 0, 220);   // Define o LED de índice 8 para azul.
  npSetLED(11, 0, 0, 220);  // Define o LED de índice 11 para azul.
  npSetLED(18, 0, 0, 220);  // Define o LED de índice 18 para azul.

  npSetLED(10, 0, 0, 220);  // Define o LED de índice 10 para azul.
}

//Pisca uma seta amarela apontando para o Botão A
void blinkYellowLed(){
  npSetBufferYellow();
  npWrite();
  playYellowSound();
  sleep_ms(LED_DISPLAY_TIME);

  npClear();
  npWrite();
}

//Pisca uma seta azul apontando para o Botão B
void blinkBlueLed(){
  npSetBufferBlue();
  npWrite();
  playBlueSound();
  sleep_ms(LED_DISPLAY_TIME);

  npClear();
  npWrite();
}

//Pisca um "Check" verde na matriz de LEDs após cada rodada pontuada.
void blinkGreenLed(){
  npSetBufferGreen();
  npWrite();
  playSuccessSound();
  sleep_ms(LED_DISPLAY_TIME);

  npClear();
  npWrite();
}

//Pisca um "X" vermelho na matriz de LEDs quando o jogador erra.
void blinkRedLed(){
  npSetBufferRed();
  npWrite();
  playErrorSound();
  sleep_ms(LED_DISPLAY_TIME);

  npClear();
  npWrite();
}

//Gera uma nova sequência.
void generateSequence() {
  srand(time(NULL));
  for (uint16_t i = 0; i < MAX_SEQUENCE_LENGTH; i++) {
    sequence[i] = rand() % 2; // 0 para Amarelo, 1 para Azul.
  }
}

// Função para exibir a sequência atual.
void displaySequence() {
  for (uint16_t i = 0; i < sequenceLength; i++) {
    switch (sequence[i]) {
      case 0:
        blinkYellowLed();
        break;
      case 1:
        blinkBlueLed();
        break;
    }
    sleep_ms(200); // Intervalo entre os LEDs.
  }
}

// Função para ler a entrada do jogador.
bool readInput() {
  for (uint16_t i = 0; i < sequenceLength; i++) {
    bool buttonPressed = false;
    while (!buttonPressed) {
      if (!gpio_get(BTN_PIN)) {
        blinkYellowLed();
        if (sequence[i] != 0){
          totalScore += i * 10;
          return false;
        } 
        buttonPressed = true;
        sleep_ms(BUTTON_DEBOUNCE_TIME);
      }
      if (!gpio_get(BTN_PIN2)) {
        blinkBlueLed();
        if (sequence[i] != 1) {
          totalScore += i * 10;
          return false;
        }
        buttonPressed = true;
        sleep_ms(BUTTON_DEBOUNCE_TIME);
      }
    }
  }
  totalScore += 100 + (sequenceLength * 10);
  return true;
}

int main() {
  // Inicializa entradas e saídas.
  stdio_init_all();

  //Inicializa o botão.
  btnInit();

  //Inicializa o buzzer.
  buzzerInit();

  // Inicializa matriz de LEDs NeoPixel.
  npInit(LED_PIN);
  npClear(); //Limpa o buffer da matriz de LEDs.

  //Gera a sequência para o jogador.
  generateSequence();

  printf("Iniciando...\n");
  sleep_ms(3000);
  printf("Bom Jogo! :)\n");
  while (true) {
    sequenceLength++;
    displaySequence();

    if (!readInput()) {
      // Jogador errou.
      printf("Game Over! :(\n");
      printf("Seu score foi de: %d pontos\n", totalScore);
      for (int i = 0; i < 3; i++) {
        blinkRedLed();
        sleep_ms(200);
        
      }

      //Reinicia o jogo.
      sleep_ms(1000);
      printf("Reiniciando...\n");
      sleep_ms(1000);
      printf("Bom Jogo! :)\n");
      totalScore = 0;
      currentRoundScore = 0;
      sequenceLength = 0;
      generateSequence();

    } else {
      // Jogador acertou.
      printf("Sequência correta!\n");
      blinkGreenLed();
      sleep_ms(500);
    }

    sleep_ms(1000); // Intervalo entre as rodadas.
  }

  return 0;
}
