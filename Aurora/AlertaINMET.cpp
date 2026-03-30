#include "AlertaINMET.h"
#include "Config.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <UniversalTelegramBot.h>
#include <WiFiClientSecure.h>

// ================= EXTERN =================
extern UniversalTelegramBot bot;

// ================= VARIÁVEIS =================
bool alertaOficialAtivo = false;
String alertaOficialMsg = "";

// controle interno
static String ultimoAlertaID = "";
static bool alertaJaEnviado = false;

// ================= FUNÇÃO PRINCIPAL =================
void verificarAlertaINMET(){
  
  if(WiFi.status() != WL_CONNECTED) return;

  static WiFiClientSecure clienteSeguro;  // ← static: alocado 1x, não na stack
  clienteSeguro.setInsecure();

  HTTPClient http;
  http.setTimeout(8000);

if(!http.begin(clienteSeguro, "https://apiprevmet3.inmet.gov.br/avisos")){
    Serial.println("Erro INMET begin");
    return;
}

  int code = http.GET();

  if(code <= 0){
    Serial.println("Erro conexão INMET");
    http.end();
    return;
  }

  if(code != 200){
    Serial.println("HTTP INMET: " + String(code));
    http.end();
    return;
  }

  String payload = http.getString();
  http.end();

  // Filtro: extrai apenas os 5 campos necessários — economiza ~8KB de heap
  StaticJsonDocument<96> filtro;
  filtro[0]["ID"]         = true;
  filtro[0]["UF"]         = true;
  filtro[0]["MUNICIPIO"]  = true;
  filtro[0]["MUN"]        = true;
  filtro[0]["DS"]         = true;
  filtro[0]["SEVERIDADE"] = true;

  DynamicJsonDocument doc(4096);
  if(deserializeJson(doc, payload, DeserializationOption::Filter(filtro))){
    Serial.println("Erro JSON INMET");
    return;
  }

  alertaOficialAtivo = false;

  for(JsonObject aviso : doc.as<JsonArray>()){

    String id = aviso["ID"] | "";
    String uf = aviso["UF"] | "";
    String municipio = aviso["MUNICIPIO"] | aviso["MUN"] | "";
    String descricao = aviso["DS"] | "";
    String severidade = aviso["SEVERIDADE"] | "";

    // normaliza
    municipio.toUpperCase();
    descricao.toUpperCase();

    // filtro MG + Muriaé / região
    bool ehMG = (uf == "MG");

    bool ehMuriae = 
        municipio.indexOf("MURIAE") >= 0 ||
        descricao.indexOf("MURIAE") >= 0 ||
        descricao.indexOf("ZONA DA MATA") >= 0;

    if(!(ehMG && ehMuriae)) continue;

    // só alertas relevantes
    if(severidade == "PERIGO" || severidade == "GRANDE PERIGO"){

      alertaOficialAtivo = true;

      // evita repetir mesmo alerta
      if(id == ultimoAlertaID){
        return;
      }

      ultimoAlertaID = id;

      alertaOficialMsg = "ALERTA INMET\n";
      alertaOficialMsg += descricao + "\n";
      alertaOficialMsg += "Nivel: " + severidade;

      Serial.println("ALERTA INMET DETECTADO");

      // envia telegram só 1x
      if(!alertaJaEnviado){
        String msg;
       msg  = "🚨 *ALERTA OFICIAL - INMET*\n";
       msg += "━━━━━━━━━━━━━━━\n\n";
       msg += alertaOficialMsg + "\n\n";
       msg += "📍 Muriaé - MG";
       bot.sendMessage(MEU_ID, msg, "Markdown");
        alertaJaEnviado = true;
      }

      return;
    }
  }

  // reset se não houver alerta
  alertaJaEnviado = false;
}