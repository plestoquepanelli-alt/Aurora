#include "Sistema.h"
#include "Config.h"
#include "LEDControl.h"
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoOTA.h>
#include "esp_system.h"

// ================= VARIÁVEIS =================
temperature_sensor_handle_t temp_handle = NULL;
unsigned long lastWiFiAttempt = 0;
bool modoNoturnoHabilitado = true;
uint8_t modoNoturnoInicioHora = 22;
uint8_t modoNoturnoFimHora = 8;

// OTA
static bool          _otaAtivo    = false;
static unsigned long _otaInicio   = 0;
#define OTA_TIMEOUT_MS  300000UL   // 5 minutos

// Clientes externos
extern WiFiClientSecure secured_client;
extern WiFiClientSecure gemini_client;
extern UniversalTelegramBot bot;

// ================= HEAP =================
void otimizarHeap(){
  delay(0); yield();
  secured_client.flush();
  gemini_client.flush();
}

int heapPercentual(){
  return (ESP.getFreeHeap() * 100) / ESP.getHeapSize();
}

// ================= WIFI =================
int wifiPercentual(){
  int rssi = WiFi.RSSI();
  if(rssi <= -100) return 0;
  if(rssi >= -50)  return 100;
  return 2 * (rssi + 100);
}

String barraVisual(int pct){
  int blocos = pct / 10;
  String barra = "";
  for(int i = 0; i < 10; i++)
    barra += (i < blocos) ? "▓" : "░";
  return barra;
}

String indicadorCor(int pct){
  if(pct > 60) return "🟢";
  if(pct > 30) return "🟡";
  return "🔴";
}

String limitarTelegram(const String &t, size_t maxLen){
  if(t.length() <= maxLen) return t;
  return t.substring(0, maxLen) + "\n\n...[truncado]";
}

String uptime(){
  unsigned long s = (millis() - bootTime) / 1000;
  char buf[20];
  sprintf(buf, "%02lu:%02lu:%02lu", s/3600, (s%3600)/60, s%60);
  return String(buf);
}

void conectarWiFi(){
  WiFi.setAutoReconnect(true);
  WiFi.persistent(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  currentState = WIFI_CONNECTING;
}

void checkWiFi(){
  if(WiFi.status() == WL_CONNECTED){
    if(currentState == WIFI_CONNECTING){
      currentState = IDLE;
      Serial.println("[WiFi] Reconectado: " + WiFi.localIP().toString());
    }
  } else {
    currentState = WIFI_CONNECTING;
  }
}

// ================= NTP =================
void sincronizarHorario(){
  configTzTime("BRT3", "pool.ntp.org", "time.nist.gov", "time.google.com");
}

String horaAtual(){
  struct tm ti;
  if(!getLocalTime(&ti)) return "Sincronizando...";
  char buf[8];
  strftime(buf, sizeof(buf), "%H:%M", &ti);
  return String(buf);
}

String dataAtual(){
  struct tm ti;
  if(!getLocalTime(&ti)) return "--/--/--";
  char buf[10];
  strftime(buf, sizeof(buf), "%d/%m/%y", &ti);
  return String(buf);
}

// ================= TEMPERATURA =================
void initTemperatura(){
  // Faixa ampliada: ESP32-S3 pode atingir 85°C sob carga
  temperature_sensor_config_t cfg = TEMPERATURE_SENSOR_CONFIG_DEFAULT(20, 100);
  temperature_sensor_install(&cfg, &temp_handle);
  temperature_sensor_enable(temp_handle);
}

float lerTemperatura(){
  float temp;
  if(temperature_sensor_get_celsius(temp_handle, &temp) != ESP_OK) return NAN;
  return temp;
}

// ================= MENU =================
void enviarMenu(){
  // Teclado inline principal — substituído pelo sistema de submenus
  // do TelegramHandler. Esta função envia só a mensagem de boas-vindas.
  String teclado = "["
    "[{\"text\":\"📊 Sistema\",\"callback_data\":\"menu_sistema\"},"
     "{\"text\":\"🌦 Clima\",\"callback_data\":\"menu_clima\"}],"
    "[{\"text\":\"🤖 IA Gemini\",\"callback_data\":\"menu_ia\"},"
     "{\"text\":\"📅 Agenda\",\"callback_data\":\"menu_agenda\"}],"
    "[{\"text\":\"📈 Relatório\",\"callback_data\":\"cmd_relatorio\"},"
     "{\"text\":\"🔐 Admin\",\"callback_data\":\"menu_admin\"}]"
  "]";

  String msg;
  msg  = "🤖 *Aurora ESP32 · AI Assistant*\n";
  msg += "━━━━━━━━━━━━━━━\n\n";
  msg += "🚀 _Sistema por: Pedro Panelli_\n\n";
  msg += "🧠 Modelo: *" + modeloAtivo + "*\n";
  msg += "⏱ Uptime: `" + uptime() + "`\n\n";
  msg += "Escolha uma categoria 👇";

  bot.sendMessageWithInlineKeyboard(MEU_ID, msg, "Markdown", teclado);
}

// ═══════════════════════════════════════════════════════════════
//  OTA — Atualização de firmware via WiFi
//  Ativado por 5 minutos a pedido do admin via Telegram
// ═══════════════════════════════════════════════════════════════
void ativarOTA(){
  if(_otaAtivo) return; // já está ativo

  ArduinoOTA.setHostname("aurora-esp32");
  // Senha OTA = mesma senha de admin do Telegram
  ArduinoOTA.setPassword(ADMIN_SENHA);

  ArduinoOTA.onStart([](){
    Serial.println("[OTA] Iniciando atualização...");
    currentState = PROCESSING;
  });

  ArduinoOTA.onEnd([](){
    Serial.println("\n[OTA] Concluído.");
    currentState = SUCCESS;
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total){
    Serial.printf("[OTA] %u%%\n", progress * 100 / total);
  });

  ArduinoOTA.onError([](ota_error_t error){
    Serial.printf("[OTA] Erro[%u]\n", error);
    currentState = ERROR_STATE;
  });

  ArduinoOTA.begin();
  _otaAtivo  = true;
  _otaInicio = millis();

  Serial.printf("[OTA] Ativo por %lu min. IP: %s\n",
    OTA_TIMEOUT_MS/60000, WiFi.localIP().toString().c_str());
}

void loopOTA(){
  if(!_otaAtivo) return;

  // Desativa após timeout
  if(millis() - _otaInicio > OTA_TIMEOUT_MS){
    _otaAtivo = false;
    ArduinoOTA.end();
    Serial.println("[OTA] Timeout — desativado.");
    return;
  }

  ArduinoOTA.handle();
}

bool otaAtivo(){
  return _otaAtivo;
}

bool isModoNoturnoAgora(){
  if(!modoNoturnoHabilitado) return false;
  struct tm t;
  if(!getLocalTime(&t)) return false;
  int h = t.tm_hour;
  if(modoNoturnoInicioHora == modoNoturnoFimHora) return true;
  if(modoNoturnoInicioHora > modoNoturnoFimHora){
    return (h >= modoNoturnoInicioHora || h < modoNoturnoFimHora);
  }
  return (h >= modoNoturnoInicioHora && h < modoNoturnoFimHora);
}

String faixaModoNoturno(){
  char buf[16];
  snprintf(buf, sizeof(buf), "%02uh–%02uh", modoNoturnoInicioHora, modoNoturnoFimHora);
  return String(buf);
}
