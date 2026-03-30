#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Adafruit_SH110X.h>

// ================= DISPLAY =================
extern Adafruit_SH1106G display;

// ================= FUNÇÕES PÚBLICAS =================
void initDisplay();
void atualizarDisplay();
void exibirAgendaOLED(const String& eventos, int diaAtual);

#endif
