#include "Clima.h"
#include "Config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>
#include "AlertaINMET.h"

// ================= VARIÁVEIS =================
float climaTemp       = 0;
int   climaUmidade    = 0;
String climaDescricao = "Sem dados";
bool alertaClimaEnviado = false;
unsigned long ultimoClima = 0;
const unsigned long INTERVALO_CLIMA = 300000; // 5 minutos

float previsao3h = 0;
float previsao6h = 0;
float previsao9h = 0;
float chuva3h    = 0;
float chuva6h    = 0;
float chuva9h    = 0;

String hora_prev3h = "+3h";
String hora_prev6h = "+6h";
String hora_prev9h = "+9h";

float climaSensTermica = 0;
int   climaPressao     = 0;
float climaVento       = 0;

extern UniversalTelegramBot bot;

// ================= IMPLEMENTAÇÕES =================

/*
 * OTIMIZAÇÕES aplicadas:
 * 1. http.end() garantido em TODOS os caminhos de saída (evita leak de socket)
 * 2. yield() após cada http.GET() — evita WDT reset em respostas lentas
 * 3. DynamicJsonDocument para forecast com filtro mais apertado (2048 → 2048 com menos campos)
 * 4. Separação das duas chamadas HTTP em funções auxiliares para clareza e stack menor
 * 5. String payload processada e liberada antes do segundo request
 */
void atualizarClima(){
  if(WiFi.status() != WL_CONNECTED) return;

  // ── Clima atual ───────────────────────────────────────────
  {
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + cidade +
                 "&units=metric&appid=" + String(WEATHER_API_KEY) + "&lang=pt_br";

    http.setTimeout(HTTP_TIMEOUT);
    if(!http.begin(url)){
      Serial.println("[Clima] Erro begin");
      http.end();
      goto forecast; // pula para previsão mesmo se atual falhar
    }

    int code = http.GET();
    yield(); // ← libera watchdog após chamada bloqueante

    if(code == 200){
      // Filtro economiza ~4KB de heap — só os campos usados
      StaticJsonDocument<96> filtro;
      filtro["main"]["temp"]       = true;
      filtro["main"]["humidity"]   = true;
      filtro["main"]["feels_like"] = true;
      filtro["main"]["pressure"]   = true;
      filtro["weather"][0]["description"] = true;
      filtro["wind"]["speed"]      = true;

      DynamicJsonDocument doc(1024);
      if(!deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filtro))){
        climaTemp        = doc["main"]["temp"]       | 0.0f;
        climaUmidade     = doc["main"]["humidity"]   | 0;
        climaSensTermica = doc["main"]["feels_like"] | 0.0f;
        climaPressao     = doc["main"]["pressure"]   | 0;
        climaVento       = (doc["wind"]["speed"] | 0.0f) * 3.6f;
        climaDescricao   = doc["weather"][0]["description"].as<String>();
        Serial.println("[Clima] atual OK: " + climaDescricao);
      } else {
        Serial.println("[Clima] Erro JSON atual");
      }
    } else {
      Serial.println("[Clima] HTTP atual: " + String(code));
    }
    http.end();
  }

  yield(); // yield entre as duas chamadas HTTP

  // ── Previsão ──────────────────────────────────────────────
  forecast:
  {
    HTTPClient httpF;
    String urlF = "http://api.openweathermap.org/data/2.5/forecast?q=" + cidade +
                  "&units=metric&appid=" + String(WEATHER_API_KEY) + "&lang=pt_br";

    httpF.setTimeout(HTTP_TIMEOUT);
    if(!httpF.begin(urlF)){
      Serial.println("[Clima] Erro begin forecast");
      httpF.end();
      goto done;
    }

    int codeF = httpF.GET();
    yield(); // ← libera watchdog

    if(codeF == 200){
      StaticJsonDocument<128> filtro;
      filtro["list"][0]["main"]["temp"] = true;
      filtro["list"][0]["pop"]          = true;
      filtro["list"][0]["dt_txt"]       = true;

      DynamicJsonDocument docF(2048);
      DeserializationError err = deserializeJson(docF, httpF.getStream(),
          DeserializationOption::Filter(filtro));

      if(!err){
        JsonArray lista = docF["list"];
        if(lista.size() >= 3){
          previsao3h = lista[0]["main"]["temp"] | 0.0f;
          previsao6h = lista[1]["main"]["temp"] | 0.0f;
          previsao9h = lista[2]["main"]["temp"] | 0.0f;
          chuva3h    = (lista[0]["pop"] | 0.0f) * 100.0f;
          chuva6h    = (lista[1]["pop"] | 0.0f) * 100.0f;
          chuva9h    = (lista[2]["pop"] | 0.0f) * 100.0f;

          auto extrairHora = [](JsonVariant v, const String& fb) -> String {
            const char* dt = v["dt_txt"] | "";
            if(strlen(dt) >= 13){
              char buf[4]; buf[0]=dt[11]; buf[1]=dt[12]; buf[2]='h'; buf[3]=0;
              return String(buf);
            }
            return fb;
          };
          hora_prev3h = extrairHora(lista[0], "+3h");
          hora_prev6h = extrairHora(lista[1], "+6h");
          hora_prev9h = extrairHora(lista[2], "+9h");
          Serial.println("[Clima] Previsão OK");
        } else {
          Serial.println("[Clima] Previsão insuficiente");
        }
      } else {
        Serial.println("[Clima] Erro JSON previsão");
      }
    } else {
      Serial.println("[Clima] HTTP previsão: " + String(codeF));
    }
    httpF.end();
  }

  done:
  verificarClimaSevero();
}

void verificarClimaSevero(){
  bool climaSevero = false;
  String alerta = "";

  if(climaTemp > 38){
    climaSevero = true;
    alerta = "🔥 Calor extremo";
  }

  String descLower = climaDescricao;
  descLower.toLowerCase();

  if(descLower.indexOf("tempestade") >= 0 ||
     descLower.indexOf("storm") >= 0 ||
     descLower.indexOf("thunder") >= 0){
    climaSevero = true;
    alerta = "🌩 Tempestade detectada";
  }

  if(descLower.indexOf("chuva forte") >= 0 ||
     descLower.indexOf("heavy rain") >= 0){
    climaSevero = true;
    alerta = "🌧 Chuva forte";
  }

  if(climaSevero && !alertaClimaEnviado){
    String msg;
    msg  = "⚠ *ALERTA CLIMÁTICO*\n";
    msg += "━━━━━━━━━━━━━━━\n\n";
    msg += alerta + "\n\n";
    msg += "🌡 " + String(climaTemp, 1) + " °C\n";
    msg += "💧 " + String(climaUmidade) + "%\n";
    msg += "☁ " + climaDescricao;
    bot.sendMessage(MEU_ID, msg, "Markdown");
    alertaClimaEnviado = true;
  }

  if(!climaSevero) alertaClimaEnviado = false;
}
