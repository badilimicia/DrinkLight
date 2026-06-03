# DrinkLight - contesto per LLM

Questo file riassume il progetto per un LLM che deve leggere o modificare la repo.

## Obiettivo

DrinkLight e' un puck smart per borraccia. La borraccia viene appoggiata sopra una cella di carico; il firmware misura quanta acqua viene bevuta e usa un anello WS2812B RGB piu' un motorino a vibrazione per ricordare quando bere durante una sessione di lavoro.

Il target attuale e' solo ESP32.

## Hardware

Componenti principali:

- ESP32.
- Anello LED WS2812B/NeoPixel.
- Cella di carico con HX711.
- Motorino vibrazione pilotato da transistor/MOSFET.

Pin attuali in `HardwareConfig.h`:

- LED: GPIO 25.
- HX711 DOUT: GPIO 21.
- HX711 SCK: GPIO 22.
- Vibrazione: GPIO 5.
- LED count: 8.

La repo non mantiene piu' compatibilita' con Arduino UNO/Nano o ESP8266.

## Configurazione

File principali:

- `UserConfig.h`: impostazioni utente, profili acqua, Wi-Fi, OTA, logging, gesture, luci.
- `HardwareConfig.h`: pin, calibrazione HX711, soglie peso/tap, timing vibrazione.
- `Config.h`: include ponte e guardia ESP32-only.

`Config.h` contiene un errore esplicito se si compila per schede non ESP32.

## Architettura firmware

Lo sketch principale `DrinkLight.ino` deve rimanere sottile. Orchestration only:

1. aggiorna servizi remoti;
2. legge bilancia;
3. legge tap;
4. legge comandi seriali/Telnet;
5. aggiorna tracking idratazione;
6. renderizza LED;
7. aggiorna vibrazione;
8. stampa/logga stato.

Moduli:

- `ScaleManager.*`: HX711, tara, scale factor, letture grammi/raw, stabilizzazione.
- `HydrationTracker.*`: sessione, baseline bottiglia, riconoscimento sorsi, refill, cambio bottiglia, pause, profili.
- `LedRing.*`: effetti LED e feedback comandi.
- `TapInput.*`: rilevamento tap dalla cella di carico.
- `VibrationMotor.*`: gestione motorino non bloccante.
- `SerialConsole.*`: parser comandi, debug, calibrazione, output USB/Telnet.
- `RemoteServices.*`: Wi-Fi, ArduinoOTA, console Telnet.

## Concetti importanti

### Tara bilancia vs baseline bottiglia

Non confondere:

- Tara/zero HX711: calibrazione hardware, salvata in `SCALE_ZERO_OFFSET`.
- Baseline bottiglia: primo peso stabile della bottiglia nella sessione.

La bottiglia puo' essere vuota, mezza piena o piena. Il primo peso stabile diventa baseline. Le bevute vengono rilevate come calo di peso dopo sollevamento e riappoggio.

### Sessione sempre alimentata

Il puck puo' restare sempre alimentato.

Il firmware non deve ricalibrare la bilancia ogni giorno. Deve riavviare solo la sessione dopo inattivita':

- `AUTO_RESTART_SESSION_ENABLED`
- `AUTO_RESTART_AFTER_IDLE_HOURS`

Reset manuale sessione: tap configurato o comando seriale `reset`.

### Sorsi dinamici

`USE_DYNAMIC_SIP_SIZE`:

- `true`: il sistema puo' suggerire sorsi piu' grandi/piccoli in base al ritmo.
- `false`: conta comunque i ml bevuti e classifica il sorso, ma i reminder seguono solo il tempo.

### Segno peso

Se appoggiando peso si leggono grammi negativi, usare:

```cpp
static const bool SCALE_INVERT_SIGN = true;
```

in `HardwareConfig.h`.

## Wi-Fi, OTA e console remota

Il firmware ESP32 usa:

- Wi-Fi STA su rete esistente.
- ArduinoOTA per upload sketch via rete.
- Telnet su `REMOTE_CONSOLE_PORT` per console remota.

Configurazione in `UserConfig.h`:

- `REMOTE_SERVICES_ENABLED`
- `WIFI_SSID`
- `WIFI_PASSWORD`
- `WIFI_FALLBACK_SSID`
- `WIFI_FALLBACK_PASSWORD`
- `OTA_HOSTNAME`
- `OTA_PASSWORD`
- `REMOTE_CONSOLE_PORT`
- `WIFI_CONNECT_TIMEOUT_SEC`
- `REMOTE_CONSOLE_FULL_LOGGING`

Il primo upload deve essere via USB. Dopo, Arduino IDE dovrebbe mostrare una porta di rete OTA per `OTA_HOSTNAME`.

La connessione Wi-Fi prova prima `WIFI_SSID`; se non si collega entro `WIFI_CONNECT_TIMEOUT_SEC`, prova `WIFI_FALLBACK_SSID`.

Telnet:

```text
telnet <ip> 23
```

La console Telnet accetta gli stessi comandi della seriale USB.

`REMOTE_CONSOLE_FULL_LOGGING = true` duplica anche il logging periodico su Telnet.
Con `false`, Telnet riceve comunque risposte ai comandi ed eventi importanti: cambi stato, bottiglia, sorsi e tap.

## Comandi console

Comandi principali:

```text
help        mostra help
status      stato completo
grams       peso in grammi
raw         letture grezze HX711
light       test colori LED
vib         test vibrazione
progress    mostra avanzamento
pause       pausa/riprendi
meeting     meeting mode
profile     cambia profilo
reset       reset sessione
end         fine giornata
tare        tara bilancia
cal 500     calibrazione rapida con peso noto
cal2 0      primo punto calibrazione
cal2 500    secondo punto calibrazione
cal2 reset  annulla calibrazione a due punti
calauto     calibrazione guidata a 3 punti con feedback LED
```

Comandi brevi:

```text
l light
g grams
s status
t tare
v vib
r reset
```

## Calibrazione

Metodo consigliato:

1. togliere tutto dal puck;
2. inviare `cal2 0`;
3. appoggiare peso noto, esempio 500 g;
4. inviare `cal2 500`;
5. copiare in `HardwareConfig.h`:
   - `SCALE_ZERO_OFFSET`
   - `SCALE_CALIBRATION_FACTOR`
6. ricaricare firmware;
7. verificare con `grams`.

Metodo rapido:

- `tare`
- `cal <grammi>`

Metodo guidato:

- `calauto`
- inserire tre pesi nominali quando richiesto, idealmente 0 g, un peso medio e un peso alto;
- appoggiare ogni peso entro 10 secondi;
- il LED mostra prompt, assestamento, sampling, successo o errore;
- il firmware calcola offset e fattore con regressione sui tre punti;
- copiare `SCALE_ZERO_OFFSET` e `SCALE_CALIBRATION_FACTOR`.

## LED

`LedRing.*` gestisce:

- stato vuoto;
- pausa;
- meeting;
- refill;
- sorso registrato;
- reminder ramp;
- due;
- overdue;
- complete;
- day complete;
- feedback comandi.

`light` / tap light show fa test colori completo:

- rosso;
- verde;
- blu;
- bianco;
- giallo;
- magenta;
- ciano;
- arancio.

## Gesture tap

La cella di carico rileva tap verticali/spinte rapide.
Con `TAP_ONLY_WHEN_BOTTLE_MISSING = true`, i tap vengono rilevati solo sul puck vuoto:
bisogna togliere la borraccia, aspettare la stabilizzazione e poi fare pressioni brevi.
Questo rende le gesture piu' affidabili e permette di uscire dalla pausa ripetendo 4 tap sul puck vuoto.

Configurazione in `UserConfig.h`:

- `TAP_COMMANDS_ENABLED`
- `TAP_COUNT_PROGRESS`
- `TAP_COUNT_PAUSE`
- `TAP_COUNT_MEETING`
- `TAP_COUNT_LIGHT_SHOW`
- `TAP_COUNT_NEXT_PROFILE`
- `TAP_COUNT_RESET_SESSION`
- `TAP_COUNT_END_DAY`

Soglie tecniche in `HardwareConfig.h`:

- `BOTTLE_PRESENT_SETTLE_DELTA_GRAMS`
- `BOTTLE_PRESENT_REQUIRED_SAMPLES`
- `TAP_DELTA_GRAMS`
- `TAP_MAX_DELTA_GRAMS`
- `TAP_RELEASE_DELTA_GRAMS`
- `TAP_SEQUENCE_GAP_MS`
- `TAP_SEQUENCE_MAX_MS`
- `TAP_REFRACTORY_MS`

I tap sono comodi ma potenzialmente meno affidabili di un pulsante IP67, sensore Hall/reed o IMU.

## Logging e performance

La compilazione ESP32 e' lenta perche' include Wi-Fi, Networking, ArduinoOTA, mDNS, Update, NeoPixel e HX711.

Per ridurre problemi:

- compilare solo per ESP32;
- non cambiare board continuamente;
- evitare build pulite non necessarie;
- usare `Core Debug Level: None`;
- non usare partition scheme `No OTA` se si vuole OTA;
- escludere repo e cache Arduino da antivirus se possibile.

Nota vista durante sviluppo: Arduino CLI su Windows puo' avere race/lock nella cache in `AppData\Local\arduino\sketches`, generando errori su file `.d` mancanti o "file usato da altro processo". Se succede, chiudere IDE/processi build e rilanciare.

## Documentazione

Guide principali:

- `README.md`: panoramica generale.
- `docs/ISTRUZIONI_OTA_CONSOLE_CALIBRAZIONE.md`: guida operativa passo passo.
- `docs/hardware.md`: cablaggio e note hardware.
- `docs/requirements.md`: requisiti funzionali/non funzionali.

Il vecchio `docs/instructions.md` e' stato rimosso per evitare doppioni.

## Stato attuale noto

- Target unico: ESP32.
- Telnet logging completo richiesto dall'utente.
- OTA attivo tramite ArduinoOTA.
- Console remota Telnet custom, non e' il Serial Monitor di Arduino IDE.
- Selezione via tap disponibile ma non considerata input primario affidabile.
- `logs/xvba_debug.log` e `.theia/` risultano nel workspace ma non sono parte del firmware.
