#ifndef CONFIG_H
#define CONFIG_H

// ================= CREDENCIAIS =================
#define WIFI_SSID       "imicro_3696-2800"
#define WIFI_PASSWORD   "Quintadasflores1"
#define BOT_TOKEN       "8600036831:AAFxt60zTolunCXtjUIgPaiDo-GXc9S9BjQ"
#define MEU_ID          "5401651820"
#define GEMINI_API_KEY  "AIzaSyDtIn8VPo1v2FoXbIbfhqI9sdJSo9B_wfw"
#define WEATHER_API_KEY "bae8fd6f5bd0b6fa4e6d55fb7ea3d9a2"

// ================= PINOS =================
#define LED_PIN         48
#define SDA_PIN         8
#define SCL_PIN         9

// Pinos SD Card
#define SD_CS           10
#define SD_MOSI         11
#define SD_MISO         13
#define SD_SCK          12

// ================= CONSTANTES =================
#define ADMIN_SENHA     "2918"
#define BOT_INTERVAL        2500
#define WIFI_RETRY_INTERVAL 10000
#define HTTP_TIMEOUT        15000
#define HEAP_MINIMO         30000
#define LED_UPDATE_INTERVAL 15

// ================= VARIÁVEIS GLOBAIS =================
extern String modeloAtivo;
extern String cidade;
extern unsigned long perguntasFeitas;
extern unsigned long bootTime;

#endif
