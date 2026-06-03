# DrinkLight - note hardware

Per la procedura passo-passo di configurazione Wi-Fi, OTA, console remota e calibrazione vedi `docs/ISTRUZIONI_OTA_CONSOLE_CALIBRAZIONE.md`.

## Scheda consigliata

DrinkLight ora e' pensato solo per ESP32: Wi-Fi, OTA, console remota e memoria disponibile rendono il firmware piu' semplice da mantenere.

## Collegamenti di base

### WS2812B / NeoPixel

- `DIN` verso `PIN_NEOPIXEL`.
- `5V` verso alimentazione LED adeguata.
- `GND` comune con microcontrollore.
- Resistenza consigliata da 330-470 ohm in serie sul dato.
- Condensatore consigliato da 470-1000 uF tra 5V e GND vicino all'anello.

Se usi ESP32 a 3.3 V con LED alimentati a 5 V, un level shifter sul dato e' consigliato.

### HX711

- `DT/DOUT` verso `PIN_HX711_DOUT`.
- `SCK` verso `PIN_HX711_SCK`.
- `VCC` secondo modulo e scheda usata.
- `GND` comune.
- Cella di carico collegata ai morsetti `E+`, `E-`, `A+`, `A-`.

La meccanica conta molto: il piano superiore deve scaricare il peso sulla cella senza sfregamenti laterali.

### Tocchi sulla cella di carico

La cella di carico puo' rilevare tap verticali o spinte rapide, quindi puo' sostituire un pulsante touch esposto all'acqua.
Con `TAP_ONLY_WHEN_BOTTLE_MISSING = true`, togli prima la borraccia, aspetta che il puck vuoto si stabilizzi e usa pressioni brevi.
Questo evita che le oscillazioni della bottiglia vengano confuse con gesture. Il firmware interpreta le sequenze cosi':

- 3 tap: mostra avanzamento;
- 4 tap: pausa/riprendi;
- 5 tap: meeting mode;
- 6 tap: gioco luci;
- 7 tap: cambia profilo sessione;
- 8 tap: reset sessione;
- 9 o piu' tap: fine giornata.

Per uscire dalla pausa manuale, togli la borraccia e ripeti 4 tap.

Limiti pratici:

- funziona meglio con una struttura rigida;
- puo' generare falsi positivi se il tavolo vibra;
- e' meno affidabile per pacche laterali;
- va tarata dopo aver scelto gommini, piano superiore e peso tipico della borraccia.

Per pacche laterali o gesture piu' affidabili, aggiungi un IMU tipo MPU-6050, LIS3DH o simile. In quel caso la cella resta dedicata al peso e l'IMU rileva urti/tilt.

Alternative piu' robuste ai tap:

- pulsante IP67 a membrana o metallico, se accetti un comando fisico;
- reed switch o sensore Hall azionato da magnete, senza contatti esposti;
- piccolo rotary/DIP interno per profilo fisso;
- BLE/Wi-Fi con ESP32 per configurazione da telefono;
- IMU per gesture laterali o doppia pacchetta.

### Motorino vibrazione

Non collegare il motorino direttamente al pin.

Schema consigliato:

- pin MCU -> resistenza 1k -> gate/base MOSFET o transistor;
- motorino tra alimentazione e transistor;
- diodo di flyback se il motorino non e' gia' protetto;
- GND comune;
- alimentazione dimensionata per la corrente di spunto.

## Configurazione

Le impostazioni sono separate:

- `UserConfig.h`: obiettivi, reminder, gesture utente, vibrazione e stile luci.
- `HardwareConfig.h`: pin, calibrazione, soglie peso, soglie tap e timing basso livello.

## Pin di default

I pin sono in `HardwareConfig.h`.

- LED: GPIO 25
- HX711 DOUT: GPIO 21
- HX711 SCK: GPIO 22
- Vibrazione: GPIO 5

## Taratura meccanica e software

- Per il prodotto finale, fai la tara una volta e salva `SCALE_ZERO_OFFSET`.
- Lascia `SCALE_TARE_ON_BOOT = false` per evitare ricalibrazioni quotidiane.
- Usa `SCALE_TARE_ON_BOOT = true` solo durante setup, con puck vuoto.
- Puoi fare la calibrazione completa da Serial Monitor:
  - `tare`: calcola e stampa `SCALE_ZERO_OFFSET`;
  - `cal 500`: calcola e stampa `SCALE_CALIBRATION_FACTOR` usando un peso noto da 500 g;
  - `cal2 0` e poi `cal2 500`: calcola sia `SCALE_ZERO_OFFSET` sia `SCALE_CALIBRATION_FACTOR`;
  - `cal2 reset`: annulla il primo punto registrato;
  - `grams`: verifica il peso letto;
  - `raw`: mostra valori HX711 grezzi per debug.
- Non serve calibrare la bottiglia piena/vuota: il primo peso stabile viene usato come baseline.
- Se fai refill, il firmware aggiorna la baseline quando vede un aumento importante di peso.
- Usa piedini antiscivolo per evitare microspostamenti.
- Se il peso oscilla troppo, aumenta `STABLE_DELTA_GRAMS` o `STABLE_REQUIRED_SAMPLES`.
- Se l'appoggio della bottiglia genera falsi rimossi/appoggiati, regola `BOTTLE_REMOVE_GRAMS`,
  `BOTTLE_PRESENT_REQUIRED_SAMPLES`, `BOTTLE_PRESENT_SETTLE_DELTA_GRAMS`,
  `BOTTLE_MISSING_REQUIRED_SAMPLES` e `SCALE_FILTER_ALPHA`.
- Se un tap sul puck vuoto viene scambiato per una bottiglia, riduci
  `BOTTLE_PRESENT_SETTLE_DELTA_GRAMS` o aumenta `BOTTLE_PRESENT_REQUIRED_SAMPLES`.
- Se conta falsi sorsi, aumenta `MIN_SIP_ML`.
- Se ignora sorsi reali piccoli, riduci `MIN_SIP_ML`.
- Se un cambio bottiglia viene contato come sorso, riduci `BOTTLE_CHANGE_DELTA_ML`.
- Se il peso e' negativo quando appoggi qualcosa, cambia `SCALE_INVERT_SIGN`.
- Se i tap non vengono rilevati, riduci `TAP_DELTA_GRAMS`.
- Se i tap partono da soli, aumenta `TAP_DELTA_GRAMS` o riduci `TAP_MAX_DELTA_GRAMS`.
- Se le sequenze di tap si chiudono troppo presto, aumenta `TAP_SEQUENCE_GAP_MS`.
- Se un singolo colpo viene contato piu' volte, aumenta `TAP_RELEASE_DELTA_GRAMS` o `TAP_REFRACTORY_MS`.

## Evoluzione hardware utile

- Buzzer disattivabile, se serve feedback sonoro.
- Batteria LiPo con circuito di ricarica e misura tensione.
- Interruttore fisico per meeting mode.
- RTC o sincronizzazione oraria via Wi-Fi per statistiche giornaliere.
- IMU/giroscopio per tap laterali, doppio colpo e gesture piu' pulite.

## OTA e debug remoto

Per chiudere il puck e lavorare senza USB usa ESP32.

- Configura Wi-Fi e password OTA in `UserConfig.h`.
- Fai il primo upload via USB.
- Dopo l'avvio, leggi l'IP da Serial Monitor o dal router.
- Usa Arduino IDE per caricare via porta di rete OTA.
- Usa Telnet sulla porta `REMOTE_CONSOLE_PORT` per console remota.

Il firmware non mantiene piu' compatibilita' con Arduino UNO/Nano o ESP8266.
