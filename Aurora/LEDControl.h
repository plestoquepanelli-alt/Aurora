#ifndef LED_CONTROL_H
#define LED_CONTROL_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// ================= ESTADOS DO SISTEMA =================
enum SystemState {
  BOOT,
  WIFI_CONNECTING,
  IDLE,
  PROCESSING,
  SUCCESS,
  ERROR_STATE
};

extern SystemState currentState;

// ================= FUNÇÕES PÚBLICAS =================
void initLED();
void setLED(uint8_t r, uint8_t g, uint8_t b);
void ledRespiracaoLaranja();
void ledTemperaturaIdle();
void ledPiscandoWiFi();
void piscarCor(uint8_t r, uint8_t g, uint8_t b);
void updateLED();
void esperarComLED(unsigned long tempo);

// ================= VARIÁVEIS =================
extern bool ledHabilitado;
extern Adafruit_NeoPixel pixel;

#endif
