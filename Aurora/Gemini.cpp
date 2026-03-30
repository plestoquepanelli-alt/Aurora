#include "Gemini.h"
#include "Config.h"
#include "Sistema.h"
#include "LEDControl.h"
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "esp_system.h"
#include "SDCard.h"

extern WiFiClientSecure gemini_client;

// ================= IMPLEMENTAÇÕES =================
String perguntarGemini(const String &pergunta){
  if(ESP.getFreeHeap() < HEAP_MINIMO){
    currentState = ERROR_STATE;
    return "Heap baixo.";
  }

  currentState = PROCESSING;

  const int maxAttempts = 3;
  int tentativa = 0;
  int backoff = 500;

  String url = "https://generativelanguage.googleapis.com/v1beta/models/"
               + modeloAtivo + ":generateContent?key=" + String(GEMINI_API_KEY);

  while(tentativa < maxAttempts){
    tentativa++;

    HTTPClient https;
    gemini_client.setInsecure();
    https.setTimeout(HTTP_TIMEOUT);

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


    DynamicJsonDocument req(6144);
    req["system_instruction"]["parts"][0]["text"] = dataHoraAtual + " " + personalidade;


    req["generationConfig"]["maxOutputTokens"] = 1500; // ~6000 chars max
    req["generationConfig"]["temperature"]     = 0.5;  // criatividade equilibrada
    req["generationConfig"]["topP"]            = 0.9;

    JsonArray contents = req.createNestedArray("contents");

    // ── Injeta histórico de contexto ──────────────────────────
    String contexto = carregarContexto();
    if(!contexto.isEmpty() && contexto.length() < 800){
      int pos = 0;
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
      }
    }

    // ── Mensagem atual ────────────────────────────────────────
    JsonObject item = contents.createNestedObject();
    item["role"] = "user";
    item["parts"][0]["text"] = pergunta;

    String body;
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
      return "HTTP " + String(httpCode) + " " + erro;
    }

    String payload = https.getString();
    https.end();

    // ── Parse com filtro — economiza ~18KB de heap ────────────
    StaticJsonDocument<128> filtro;
    filtro["candidates"][0]["content"]["parts"][0]["text"] = true;
    filtro["error"]["message"] = true;

    // resp(6144): 1500 tokens × ~6 chars/token + overhead JSON = ~11KB raw
    // O filtro reduz para apenas o campo de texto (~4-6KB efetivo)
    // 4096 era insuficiente e truncava silenciosamente respostas longas
    DynamicJsonDocument resp(6144);
    DeserializationError err = deserializeJson(resp, payload,
      DeserializationOption::Filter(filtro));

    if(err){ currentState = ERROR_STATE; return "Erro JSON: " + String(err.c_str()); }

    String resposta = "Sem resposta";
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