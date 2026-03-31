#include "Gemini.h"
#include "Config.h"
#include "Sistema.h"
#include "LEDControl.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include "esp_system.h"
#include "SDCard.h"

extern WiFiClientSecure gemini_client;

// ================= IMPLEMENTAÇÕES =================
String perguntarGemini(const String &pergunta){
  const uint32_t heapLivre = ESP.getFreeHeap();
  const uint32_t limiarHeap = (ESP.getFreePsram() > 0) ? 18000UL : HEAP_MINIMO;
  if(heapLivre < limiarHeap){
    currentState = ERROR_STATE;
    return "Heap baixo (" + String(heapLivre) + "B).";
  }
  if(WiFi.status() != WL_CONNECTED){
    currentState = ERROR_STATE;
    return "WiFi desconectado.";
  }
  if(String(GEMINI_API_KEY).length() < 20){
    currentState = ERROR_STATE;
    return "GEMINI_API_KEY inválida/não configurada.";
  }

  currentState = PROCESSING;

  const int maxAttempts = 2;
  int tentativa = 0;
  int backoff = 300;

  String url;
  url.reserve(180);
  url = "https://generativelanguage.googleapis.com/v1beta/models/";
  url += modeloAtivo;
  url += ":generateContent?key=";
  url += GEMINI_API_KEY;

  while(tentativa < maxAttempts){
    tentativa++;

    HTTPClient https;
    gemini_client.setInsecure();
    https.setTimeout(9000);
    https.setReuse(false);

    if(!https.begin(gemini_client, url)){
      if(tentativa >= maxAttempts){ currentState = ERROR_STATE; return "Erro conexão Gemini."; }
      esperarComLED(backoff); backoff *= 2; continue;
    }

    https.addHeader("Content-Type", "application/json");

    // ── Monta data/hora atual via NTP ──────────────────────────
    String dataHoraAtual = "";
    {
      struct tm timeinfo;
      if(getLocalTime(&timeinfo)){
        char buf[48];
        strftime(buf, sizeof(buf),
          "Hoje é %d/%m/%Y. São %H:%M (horário de Brasília).", &timeinfo);
        dataHoraAtual = String(buf);
      }
    }

    // ── Personalidade do SD ou padrão ─────────────────────────
    String personalidade = carregarPersonalidade();
    if(personalidade.isEmpty())
      personalidade = "Você é Aurora, assistente pessoal de Pedro. Seja amigável, técnico e conciso.";


    DynamicJsonDocument req(4096);
    req["system_instruction"]["parts"][0]["text"] = dataHoraAtual + " " + personalidade;


    req["generationConfig"]["maxOutputTokens"] = 700;
    req["generationConfig"]["temperature"]     = 0.45;
    req["generationConfig"]["topP"]            = 0.9;

    JsonArray contents = req.createNestedArray("contents");

    // ── Injeta histórico de contexto ──────────────────────────
    String contexto = carregarContexto();
    if(!contexto.isEmpty() && contexto.length() < 500){
      int pos = 0;
      int linhas = 0;
      while(pos < (int)contexto.length()){
        int nl = contexto.indexOf('\n', pos);
        if(nl < 0) nl = contexto.length();
        String linha = contexto.substring(pos, nl);
        pos = nl + 1;
        if(linha.startsWith("P:")){
          JsonObject msg = contents.createNestedObject();
          msg["role"] = "user";
          msg["parts"][0]["text"] = linha.substring(2);
        } else if(linha.startsWith("R:")){
          JsonObject msg = contents.createNestedObject();
          msg["role"] = "model";
          msg["parts"][0]["text"] = linha.substring(2);
        }
        linhas++;
        if((linhas % 4) == 0) yield();
      }
    }

    // ── Mensagem atual ────────────────────────────────────────
    JsonObject item = contents.createNestedObject();
    item["role"] = "user";
    item["parts"][0]["text"] = pergunta;

    String body;
    body.reserve(2400);
    serializeJson(req, body);

    int httpCode = https.POST(body);

    if(httpCode < 0){
      https.end();
      if(tentativa >= maxAttempts){ currentState = ERROR_STATE; return "Erro HTTP interno."; }
      esperarComLED(backoff); backoff *= 2; continue;
    }

    if(httpCode == 429 || (httpCode >= 500 && httpCode < 600)){
      https.end();
      if(tentativa >= maxAttempts){ currentState = ERROR_STATE; return "Servidor ocupado (HTTP " + String(httpCode) + ")"; }
      esperarComLED(backoff); backoff *= 2; continue;
    }

    if(httpCode != 200){
      String erro = https.getString();
      https.end();
      currentState = ERROR_STATE;
      String msg = "HTTP " + String(httpCode);
      DynamicJsonDocument errDoc(512);
      if(deserializeJson(errDoc, erro) == DeserializationError::Ok &&
         errDoc["error"]["message"].is<const char*>()){
        msg += " " + String((const char*)errDoc["error"]["message"]);
      } else if(!erro.isEmpty()){
        msg += " " + erro.substring(0, 120);
      }
      return msg;
    }

    String payload = https.getString();
    https.end();
    if(payload.length() > 12000){
      currentState = ERROR_STATE;
      return "Resposta Gemini muito grande.";
    }

    // ── Parse com filtro — economiza ~18KB de heap ────────────
    StaticJsonDocument<128> filtro;
    filtro["candidates"][0]["content"]["parts"][0]["text"] = true;
    filtro["error"]["message"] = true;

    // resp(6144): 1500 tokens × ~6 chars/token + overhead JSON = ~11KB raw
    // O filtro reduz para apenas o campo de texto (~4-6KB efetivo)
    // 4096 era insuficiente e truncava silenciosamente respostas longas
    DynamicJsonDocument resp(4096);
    DeserializationError err = deserializeJson(resp, payload,
      DeserializationOption::Filter(filtro));

    if(err){ currentState = ERROR_STATE; return "Erro JSON: " + String(err.c_str()); }

    String resposta;
    resposta.reserve(900);
    resposta = "Sem resposta";
    if(resp["candidates"][0]["content"]["parts"][0]["text"].is<const char*>())
      resposta = String((const char*)resp["candidates"][0]["content"]["parts"][0]["text"]);

    currentState = SUCCESS;
    otimizarHeap();
    salvarContexto(pergunta, resposta);
    return resposta;
  }

  currentState = ERROR_STATE;
  return "Falha desconhecida.";
}
