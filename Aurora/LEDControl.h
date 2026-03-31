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

struct LedColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

enum LedEffect : uint8_t {
  LED_EF_RAINBOW = 0,
  LED_EF_STROBE_RAPIDO,
  LED_EF_STROBE_MEDIO,
  LED_EF_STROBE_LENTO,
  LED_EF_RESPIRACAO
};

// ================= FUNÇÕES PÚBLICAS =================
void initLED();
void setLED(uint8_t r, uint8_t g, uint8_t b);
void ledRespiracaoLaranja();
void ledTemperaturaIdle();
void ledPiscandoWiFi();
void piscarCor(uint8_t r, uint8_t g, uint8_t b);
void updateLED();
void esperarComLED(unsigned long tempo);
bool definirCorLED(const String& nome, uint8_t r, uint8_t g, uint8_t b);
LedColor obterCorLED(const String& nome);
bool definirEfeitoLED(const String& nome, const String& efeito);
String obterEfeitoLED(const String& nome);

// ================= VARIÁVEIS =================
extern bool ledHabilitado;
extern Adafruit_NeoPixel pixel;

#endif
