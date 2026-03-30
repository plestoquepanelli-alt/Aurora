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
  uint8_t brilho = 20 + (seno * 120);
  setLED(brilho, brilho * 0.5, 0);
}

void ledTemperaturaIdle(){
  float temp = lerTemperatura();
  faseBranco += 0.01;
  if(faseBranco > TWO_PI) faseBranco = 0;
  float seno = (sin(faseBranco) + 1.0) * 0.5;
  uint8_t brilho = 5 + (seno * 105);

  if(isnan(temp)){ 
    setLED(0, 0, brilho); 
    return; 
  }

  if(temp < 35) setLED(0, 0, brilho);
  else if(temp < 55) setLED(0, brilho, 0);
  else if(temp < 70) setLED(brilho, brilho, 0);
  else if(temp < 80) setLED(brilho, brilho * 0.4, 0);
  else setLED(brilho, 0, 0);
}

void ledPiscandoWiFi(){
  static bool on = false;
  if(millis() - lastBlink > 400){
    lastBlink = millis();
    on = !on;
  }
  if(on) setLED(0, 0, 80);
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
    case SUCCESS: piscarCor(0, 180, 0); break;
    case ERROR_STATE: piscarCor(180, 0, 0); break;
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
