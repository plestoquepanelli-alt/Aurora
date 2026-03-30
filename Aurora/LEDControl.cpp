#include "LEDControl.h"
#include "Config.h"
#include "Sistema.h"
#include <math.h>

// ================= VARIÁVEIS =================
Adafruit_NeoPixel pixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);
SystemState currentState = BOOT;
bool ledHabilitado = true;

unsigned long lastLedUpdate = 0;
float faseBranco = 0.0;
float faseLaranja = 0.0;
uint8_t blinkStep = 0;
unsigned long lastBlink = 0;
LedColor corWiFi       = {0, 0, 80};
LedColor corIdle       = {0, 80, 255};
LedColor corProcessing = {255, 120, 0};
LedColor corSuccess    = {0, 180, 0};
LedColor corError      = {180, 0, 0};

// ================= IMPLEMENTAÇÕES =================
void initLED(){
  pixel.begin();
  pixel.clear();
  pixel.show();
}

void setLED(uint8_t r, uint8_t g, uint8_t b){
  pixel.setPixelColor(0, pixel.Color(r, g, b));
  pixel.show();
}

void ledRespiracaoLaranja(){
  faseLaranja += 0.08;
  if(faseLaranja > TWO_PI) faseLaranja = 0;
  float seno = (sin(faseLaranja) + 1.0) * 0.5;
  float fator = 0.15f + (seno * 0.85f);
  setLED(
    (uint8_t)(corProcessing.r * fator),
    (uint8_t)(corProcessing.g * fator),
    (uint8_t)(corProcessing.b * fator)
  );
}

void ledTemperaturaIdle(){
  faseBranco += 0.01;
  if(faseBranco > TWO_PI) faseBranco = 0;
  float seno = (sin(faseBranco) + 1.0) * 0.5;
  float fator = 0.15f + (seno * 0.85f);
  setLED(
    (uint8_t)(corIdle.r * fator),
    (uint8_t)(corIdle.g * fator),
    (uint8_t)(corIdle.b * fator)
  );
}

void ledPiscandoWiFi(){
  static bool on = false;
  if(millis() - lastBlink > 400){
    lastBlink = millis();
    on = !on;
  }
  if(on) setLED(corWiFi.r, corWiFi.g, corWiFi.b);
  else setLED(0, 0, 0);
}

void piscarCor(uint8_t r, uint8_t g, uint8_t b){
  if(millis() - lastBlink > 200){
    lastBlink = millis();
    blinkStep++;
  }
  if(blinkStep >= 6){
    blinkStep = 0;
    currentState = IDLE;
    return;
  }
  if(blinkStep % 2 == 0) setLED(r, g, b);
  else setLED(0, 0, 0);
}

void updateLED(){
  if(!ledHabilitado){
    pixel.clear();
    pixel.show();
    return;
  }

  if(millis() - lastLedUpdate < LED_UPDATE_INTERVAL) return;
  lastLedUpdate = millis();

  switch(currentState){
    case WIFI_CONNECTING: ledPiscandoWiFi(); break;
    case IDLE: ledTemperaturaIdle(); break;
    case PROCESSING: ledRespiracaoLaranja(); break;
    case SUCCESS: piscarCor(corSuccess.r, corSuccess.g, corSuccess.b); break;
    case ERROR_STATE: piscarCor(corError.r, corError.g, corError.b); break;
    default: break;
  }
}

void esperarComLED(unsigned long tempo){
  unsigned long inicio = millis();
  while(millis() - inicio < tempo){
    updateLED();
    delay(1);
    yield();
  }
}

bool definirCorLED(const String& nome, uint8_t r, uint8_t g, uint8_t b){
  LedColor cor = {r, g, b};
  if(nome == "wifi") corWiFi = cor;
  else if(nome == "idle") corIdle = cor;
  else if(nome == "processing") corProcessing = cor;
  else if(nome == "success") corSuccess = cor;
  else if(nome == "error") corError = cor;
  else return false;
  return true;
}

LedColor obterCorLED(const String& nome){
  if(nome == "wifi") return corWiFi;
  if(nome == "idle") return corIdle;
  if(nome == "processing") return corProcessing;
  if(nome == "success") return corSuccess;
  if(nome == "error") return corError;
  return {0, 0, 0};
}
