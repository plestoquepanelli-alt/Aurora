#ifndef SISTEMA_H
#define SISTEMA_H

#include <Arduino.h>
#include <WiFi.h>
#include "driver/temperature_sensor.h"

// ================= FUNÇÕES PÚBLICAS =================
void otimizarHeap();
int  heapPercentual();
int  wifiPercentual();
String barraVisual(int pct);
String indicadorCor(int pct);
String limitarTelegram(const String &t, size_t maxLen = 3800);
String uptime();
void conectarWiFi();
void checkWiFi();
void sincronizarHorario();
String horaAtual();
String dataAtual();
void initTemperatura();
float lerTemperatura();
void enviarMenu();
bool isModoNoturnoAgora();
String faixaModoNoturno();
void iniciarModoConfigAP();
void pararModoConfigAP();
bool apConfigAtivo();
String nomeAPConfig();
bool conectarWiFiComCredenciais(const String& ssid, const String& senha, unsigned long timeoutMs = 20000UL);

// ================= OTA =================
void ativarOTA();          
void loopOTA();            
bool otaAtivo();           
// ================= VARIÁVEIS =================
extern temperature_sensor_handle_t temp_handle;
extern unsigned long lastWiFiAttempt;
extern bool modoNoturnoHabilitado;
extern uint8_t modoNoturnoInicioHora;
extern uint8_t modoNoturnoFimHora;

#endif
