#include "LEDControl.h"
#include "Config.h"
#include "Sistema.h"
#include <math.h>

// ================= VARIÁVEIS =================
Adafruit_NeoPixel pixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);
SystemState currentState = BOOT;
bool ledHabilitado = true;

unsigned long lastLedUpdate = 0;
uint8_t blinkStep = 0;
unsigned long lastBlink = 0;
LedColor corWiFi       = {0, 0, 80};
LedColor corIdle       = {0, 80, 255};
LedColor corProcessing = {255, 120, 0};
LedColor corSuccess    = {0, 180, 0};
LedColor corError      = {180, 0, 0};
LedEffect efWiFi       = LED_EF_STROBE_MEDIO;
LedEffect efIdle       = LED_EF_RESPIRACAO;
LedEffect efProcessing = LED_EF_RESPIRACAO;
LedEffect efSuccess    = LED_EF_STROBE_RAPIDO;
LedEffect efError      = LED_EF_STROBE_MEDIO;

static uint32_t rodaCor(uint8_t pos){
  pos = 255 - pos;
  if(pos < 85) return pixel.Color(255 - pos * 3, 0, pos * 3);
  if(pos < 170){ pos -= 85; return pixel.Color(0, pos * 3, 255 - pos * 3); }
  pos -= 170; return pixel.Color(pos * 3, 255 - pos * 3, 0);
}

static void aplicarEfeito(const LedColor& cor, LedEffect efeito){
  unsigned long agora = millis();
  switch(efeito){
    case LED_EF_RAINBOW: {
      uint8_t p = (agora / 8) & 255;
      pixel.setPixelColor(0, rodaCor(p));
      pixel.show();
      break;
    }
    case LED_EF_STROBE_RAPIDO:
    case LED_EF_STROBE_MEDIO:
    case LED_EF_STROBE_LENTO: {
      unsigned long periodo = (efeito == LED_EF_STROBE_RAPIDO) ? 90 : (efeito == LED_EF_STROBE_MEDIO ? 180 : 320);
      bool on = ((agora / periodo) % 2) == 0;
      if(on) setLED(cor.r, cor.g, cor.b);
      else setLED(0, 0, 0);
      break;
    }
    case LED_EF_RESPIRACAO:
    default: {
      float fase = (float)(agora % 3000UL) / 3000.0f;
      float seno = (sin(fase * TWO_PI) + 1.0f) * 0.5f;
      float fator = 0.15f + (seno * 0.85f);
      setLED((uint8_t)(cor.r * fator), (uint8_t)(cor.g * fator), (uint8_t)(cor.b * fator));
      break;
    }
  }
}

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
  aplicarEfeito(corProcessing, efProcessing);
}

void ledTemperaturaIdle(){
  aplicarEfeito(corIdle, efIdle);
}

void ledPiscandoWiFi(){
  aplicarEfeito(corWiFi, efWiFi);
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
    case SUCCESS: aplicarEfeito(corSuccess, efSuccess); break;
    case ERROR_STATE: aplicarEfeito(corError, efError); break;
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

static LedEffect strParaEfeito(const String& e){
  if(e == "arco-iris") return LED_EF_RAINBOW;
  if(e == "strobo_rapido") return LED_EF_STROBE_RAPIDO;
  if(e == "strobo_medio") return LED_EF_STROBE_MEDIO;
  if(e == "strobo_devagar") return LED_EF_STROBE_LENTO;
  return LED_EF_RESPIRACAO;
}

static String efeitoParaStr(LedEffect e){
  switch(e){
    case LED_EF_RAINBOW: return "arco-iris";
    case LED_EF_STROBE_RAPIDO: return "strobo_rapido";
    case LED_EF_STROBE_MEDIO: return "strobo_medio";
    case LED_EF_STROBE_LENTO: return "strobo_devagar";
    case LED_EF_RESPIRACAO:
    default: return "respiracao";
  }
}

bool definirEfeitoLED(const String& nome, const String& efeito){
  LedEffect ef = strParaEfeito(efeito);
  if(nome == "wifi") efWiFi = ef;
  else if(nome == "idle") efIdle = ef;
  else if(nome == "processing") efProcessing = ef;
  else if(nome == "success") efSuccess = ef;
  else if(nome == "error") efError = ef;
  else return false;
  return true;
}

String obterEfeitoLED(const String& nome){
  if(nome == "wifi") return efeitoParaStr(efWiFi);
  if(nome == "idle") return efeitoParaStr(efIdle);
  if(nome == "processing") return efeitoParaStr(efProcessing);
  if(nome == "success") return efeitoParaStr(efSuccess);
  if(nome == "error") return efeitoParaStr(efError);
  return "respiracao";
}
