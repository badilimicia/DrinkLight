#pragma once

#include <Arduino.h>

// ============================================================
// DrinkLight - configurazione utente
// ============================================================
// Questo e' il file da modificare normalmente.
// Per pin, calibrazione HX711 e soglie tecniche usa HardwareConfig.h.

// ------------------------------------------------------------
// 1) Profilo acqua
// ------------------------------------------------------------
// Scegli quale profilo usare all'accensione:
// 0 = Focus, 1 = Mezza giornata, 2 = Giornata intera.
static const uint8_t START_PROFILE = 1;

struct DrinkProfile {
  const char* name;
  uint16_t targetMl;
  uint16_t durationMin;
  uint16_t referenceSipMl;
};

// name, target totale, durata, sorso di riferimento.
// Il sorso di riferimento decide anche se il feedback dopo una bevuta
// e' "piccolo", "ok" o "grande". MIN_SIP_ML in HardwareConfig.h decide
// solo se una variazione di peso e' abbastanza grande da essere contata.
static const DrinkProfile DRINK_PROFILES[] = {
  { "Focus", 500, 120, 70 },
  { "HalfDay", 1000, 240, 80 },
  { "FullDay", 1800, 480, 90 }
};

static const uint8_t DRINK_PROFILE_COUNT = sizeof(DRINK_PROFILES) / sizeof(DRINK_PROFILES[0]);
static const uint8_t DEFAULT_DRINK_PROFILE_INDEX = START_PROFILE < DRINK_PROFILE_COUNT ? START_PROFILE : 0;

// ------------------------------------------------------------
// 2) Come deve ragionare sui sorsi
// ------------------------------------------------------------
// true:
//   se sei in ritardo puo' suggerire sorsi un po' piu' grandi.
// false:
//   conta comunque quanto bevi, ma i reminder seguono solo il tempo.
static const bool USE_DYNAMIC_SIP_SIZE = true;

static const uint16_t MIN_RECOMMENDED_SIP_ML = 25;
static const uint16_t MAX_RECOMMENDED_SIP_ML = 150;

// ------------------------------------------------------------
// 3) Reminder e pause
// ------------------------------------------------------------
static const uint16_t REMINDER_RAMP_SECONDS = 300;      // Luce graduale prima del sorso.
static const uint16_t OVERDUE_AFTER_SECONDS = 120;      // Ritardo prima dello stato "in ritardo".
static const uint16_t AUTO_PAUSE_AFTER_SECONDS = 900;   // Pausa automatica se manca la borraccia.
static const uint16_t MEETING_MODE_MINUTES = 30;        // Durata modalita meeting.

// ------------------------------------------------------------
// 3B) Inizio automatico di una nuova sessione
// ------------------------------------------------------------
// Il puck resta sempre alimentato: dopo molte ore senza eventi utili
// azzera solo il contatore della sessione, NON ricalibra la bilancia.
static const bool AUTO_RESTART_SESSION_ENABLED = true;
static const uint8_t AUTO_RESTART_AFTER_IDLE_HOURS = 10;

// ------------------------------------------------------------
// 4) Luci e vibrazione
// ------------------------------------------------------------
enum class LightStyle {
  Calm,   // Sobrio: poco invasivo.
  Vivid   // Piu' scenografico: comete e highlight, ma stessi colori.
};

static const LightStyle LIGHT_STYLE = LightStyle::Vivid;
static const uint8_t LED_BRIGHTNESS = 80;               // Luminosita normale, 0..255.
static const uint8_t LED_VIVID_BRIGHTNESS = 140;        // Limite in stile Vivid.
static const bool VIBRATION_ENABLED = false;

// ------------------------------------------------------------
// 5) Comandi via tap
// ------------------------------------------------------------
// Se la selezione via tap risulta scomoda, puoi disattivare tutto qui
// e usare i comandi seriali in TEST_MODE.
static const bool TAP_COMMANDS_ENABLED = true;
// Sul puck vuoto le gesture sono piu' affidabili: la bottiglia non oscilla
// e il riappoggio non puo' essere confuso con una sequenza.
static const bool TAP_ONLY_WHEN_BOTTLE_MISSING = true;

// Mappa attuale. Con TAP_ONLY_WHEN_BOTTLE_MISSING=true,
// togli prima la borraccia e fai pressioni brevi sul puck vuoto:
// 3 tap = avanzamento
// 4 tap = pausa/riprendi
// 5 tap = meeting mode
// 6 tap = gioco luci
// 7 tap = cambia profilo
// 8 tap = reset sessione
// 9+ tap = fine giornata
static const uint8_t TAP_COUNT_PROGRESS = 3;
static const uint8_t TAP_COUNT_PAUSE = 4;
static const uint8_t TAP_COUNT_MEETING = 5;
static const uint8_t TAP_COUNT_LIGHT_SHOW = 6;
static const uint8_t TAP_COUNT_NEXT_PROFILE = 7;
static const uint8_t TAP_COUNT_RESET_SESSION = 8;
static const uint8_t TAP_COUNT_END_DAY = 9;

// ------------------------------------------------------------
// 6) Modalita test
// ------------------------------------------------------------
// Off:
//   uso normale.
// Serial:
//   stampa piu' dettagli e abilita comandi da Serial Monitor.
// Hardware:
//   feedback fisico piu' evidente per testare LED/tap/vibrazione.
// Full:
//   Serial + Hardware.
enum class TestMode {
  Off,
  Serial,
  Hardware,
  Full
};

static const TestMode TEST_MODE = TestMode::Full;
static const uint16_t TEST_SERIAL_INTERVAL_MS = 100;
static const bool TEST_VIBRATE_ON_TAP = false;

// ------------------------------------------------------------
// 7) Wi-Fi, OTA e console remota
// ------------------------------------------------------------
// DrinkLight e' ESP32-only. Questi servizi sono parte del target principale.
static const bool REMOTE_SERVICES_ENABLED = true;
static const char* WIFI_SSID = "broggiWifi";
static const char* WIFI_PASSWORD = "testtyyuu";
static const char* WIFI_FALLBACK_SSID = "Broggi_WiFi_2.4";
static const char* WIFI_FALLBACK_PASSWORD = "BroggiWifi11?";
static const char* OTA_HOSTNAME = "drinklight";
static const char* OTA_PASSWORD = "testtyyuu";
static const uint16_t REMOTE_CONSOLE_PORT = 23;
static const uint16_t WIFI_CONNECT_TIMEOUT_SEC = 20;

// true:
//   la console Telnet riceve anche il logging periodico completo.
// false:
//   Telnet riceve risposte ai comandi ed eventi importanti
//   (cambi stato, bottiglia, sorsi e tap), ma non il log periodico.
static const bool REMOTE_CONSOLE_FULL_LOGGING = false;

// ------------------------------------------------------------
// Alias interni: non serve modificarli.
// ------------------------------------------------------------
static const uint16_t SESSION_TARGET_ML = DRINK_PROFILES[DEFAULT_DRINK_PROFILE_INDEX].targetMl;
static const uint16_t SESSION_DURATION_MIN = DRINK_PROFILES[DEFAULT_DRINK_PROFILE_INDEX].durationMin;
static const uint16_t EXPECTED_SIP_ML = DRINK_PROFILES[DEFAULT_DRINK_PROFILE_INDEX].referenceSipMl;
static const uint16_t RAMP_BEFORE_DUE_SEC = REMINDER_RAMP_SECONDS;
static const uint16_t OVERDUE_GRACE_SEC = OVERDUE_AFTER_SECONDS;
static const uint16_t AUTO_PAUSE_MISSING_SEC = AUTO_PAUSE_AFTER_SECONDS;
static const uint16_t MEETING_MODE_MIN = MEETING_MODE_MINUTES;
