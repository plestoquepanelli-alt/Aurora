#ifndef GEMINI_TYPES_H
#define GEMINI_TYPES_H

// ════════════════════════════════════════════════════════════
//  GEMINI ASYNC — tipos compartilhados entre todos os módulos
//
//  Este header é a ÚNICA definição de GJobState e GeminiJob.
//  Inclua-o em: Aurora.ino, Display.cpp, TelegramHandler.cpp
//
//  Problema anterior: enum GJobState era redeclarado 3 vezes
//  (Aurora.ino L91, Display.cpp L7, TelegramHandler.cpp L19)
//  causando "redefinition of 'GJobState'" no compilador.
// ════════════════════════════════════════════════════════════
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

enum GJobState : uint8_t {
    GJOB_IDLE    = 0,
    GJOB_PENDING = 1,
    GJOB_RUNNING = 2,
    GJOB_DONE    = 3
};

struct GeminiJob {
    volatile GJobState state = GJOB_IDLE;
    uint32_t jobId    = 0;
    String pergunta  = "";
    String chatId    = "";
    String resposta  = "";
    unsigned long ts = 0;
};

// Declaração extern — definição real fica em Aurora.ino
extern GeminiJob          gJob;
extern SemaphoreHandle_t  gMutex;

#endif
