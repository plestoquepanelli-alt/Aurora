#ifndef TELEGRAM_HANDLER_H
#define TELEGRAM_HANDLER_H

#include <Arduino.h>

// Processa mensagens do Telegram (polling + callbacks + estado)
void processarMensagens();
void verificarRespostaGemini();  // despacha resposta do Core 0 para Telegram

// Variáveis
extern unsigned long lastBotCheck;

#endif
