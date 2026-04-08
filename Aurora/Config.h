#ifndef CONFIG_H
#define CONFIG_H

// ================= CREDENCIAIS =================
#define WIFI_SSID       "x-x-x-x-x-x-x"
#define WIFI_PASSWORD   "x-x-x-x-x-x-x"
#define BOT_TOKEN       "x-x-x-x:x-x-x-x-x-x-x-x"
#define MEU_ID          "x-x-x-x-x-x"
#define GEMINI_API_KEY  "x-x-x-x-x-x"
#define WEATHER_API_KEY "x-x-x-x-x-x"

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
#define ADMIN_SENHA     "1234"
#define BOT_INTERVAL        2500
#define WIFI_RETRY_INTERVAL 10000
#define HTTP_TIMEOUT        6000
#define HEAP_MINIMO         30000
#define LED_UPDATE_INTERVAL 15

// ================= VARIÁVEIS GLOBAIS =================
extern String modeloAtivo;
extern String cidade;
extern unsigned long perguntasFeitas;
extern unsigned long bootTime;

#endif
