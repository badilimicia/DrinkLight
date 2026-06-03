# DrinkLight

DrinkLight e' un puck per borraccia che misura quanta acqua viene bevuta durante una sessione di lavoro e usa un anello WS2812B piu' un motorino a vibrazione per ricordare quando bere.

Il progetto e' pensato per ESP32 con Arduino IDE, senza PlatformIO: apri `DrinkLight.ino`, installa le librerie indicate sotto e carica lo sketch sulla scheda.

Per la procedura completa di uso, calibrazione, OTA e console remota vedi [docs/ISTRUZIONI_OTA_CONSOLE_CALIBRAZIONE.md](docs/ISTRUZIONI_OTA_CONSOLE_CALIBRAZIONE.md).
Per la legenda completa dell'anello LED vedi [docs/MAPPA_LUCI.md](docs/MAPPA_LUCI.md).

## Hardware previsto

- ESP32.
- Anello LED WS2812B/NeoPixel, configurato di default a 8 LED.
- Cella di carico con modulo HX711.
- Motorino a vibrazione pilotato da transistor o MOSFET.
- Alimentazione adeguata per LED e motorino.

## Librerie Arduino IDE

Installa dal Library Manager:

- `Adafruit NeoPixel`
- `HX711` di Bogdan Necula / bogde

## File principali

- `DrinkLight.ino`: setup e loop principale.
- `UserConfig.h`: target acqua, durata sessione, ritmo, gesture, vibrazione e stile luci.
- `HardwareConfig.h`: pin, calibrazione HX711, soglie peso, soglie tap e timing hardware.
- `Config.h`: include di compatibilita' che carica i due file di configurazione.
- `ScaleManager.*`: lettura e stabilizzazione peso.
- `HydrationTracker.*`: riconoscimento sorso e modello di reminder.
- `LedRing.*`: effetti luminosi.
- `RemoteServices.*`: Wi-Fi, OTA e console remota Telnet su ESP32.
- `SerialConsole.*`: comandi seriali, debug, calibrazione HX711.
- `TapInput.*`: comandi con tocchi/spinte rapide rilevati dalla cella di carico.
- `VibrationMotor.*`: impulsi del motorino.

## Comportamento base

1. Il puck usa l'offset bilancia configurato in `HardwareConfig.h`.
2. Quando appoggi la borraccia, registra un peso stabile iniziale come baseline.
3. Quando sollevi la borraccia e la riappoggi, confronta il nuovo peso stabile con quello precedente.
4. Se il peso e' diminuito di almeno `MIN_SIP_ML`, registra un sorso.
5. La luce resta spenta quando sei in linea con il piano.
6. Prima del prossimo sorso previsto, l'anello si accende gradualmente.
7. Quando il sorso e' dovuto, pulsa.
8. Quando sei in ritardo, lampeggia rosso e vibra a intervalli.
9. Se `USE_DYNAMIC_SIP_SIZE` e' attivo e bevi prima del dovuto, i ml vanno a bilancio e il reminder successivo viene spostato piu' avanti.
10. Se `USE_DYNAMIC_SIP_SIZE` e' spento, il sistema conta comunque i ml e valuta il sorso, ma il reminder segue solo il tempo dall'ultimo sorso.

## Calibrazione bottiglia

Non serve dire al puck se la bottiglia e' vuota, mezza piena o piena. Serve invece che la bilancia abbia un offset zero corretto.

Nel prodotto finale il puck puo' restare sempre alimentato:

1. Fai la tara una volta durante setup/calibrazione.
2. Copia `tare_offset` in `SCALE_ZERO_OFFSET`.
3. Lascia `SCALE_TARE_ON_BOOT = false`.
4. Il firmware non ricalibra la bilancia a ogni avvio.
5. Ogni nuova sessione riparte dalla prima baseline stabile della bottiglia.

Se il peso aumenta molto, viene trattato come refill e diventa una nuova baseline. Se il peso cala troppo rispetto a un sorso realistico, viene trattato come cambio bottiglia/anomalia e non come bevuta.

Il riavvio della sessione e' automatico: se non ci sono eventi utili per `AUTO_RESTART_AFTER_IDLE_HOURS`, il contatore riparte da zero senza toccare la calibrazione della bilancia. Puoi sempre forzare un reset con 8 tap o con il comando seriale `r` in test mode.

## Interazioni senza pulsante touch

Il firmware puo' usare la cella di carico anche come input.
Con `TAP_ONLY_WHEN_BOTTLE_MISSING = true`, togli prima la borraccia e fai i tap sul puck vuoto:

- 3 tap/spinte rapide sul puck: mostra avanzamento.
- 4 tap: pausa o riprendi.
- 5 tap: meeting mode, luce bassa e niente vibrazione.
- 6 tap: gioco luci temporaneo.
- 7 tap: cambia profilo sessione senza azzerare il contatore.
- 8 tap: reset sessione.
- 9 o piu' tap: fine giornata.

Per uscire dalla pausa manuale, togli la borraccia e ripeti la sequenza da 4 tap.

Questa funzione dipende molto dalla meccanica del puck e va tarata con:

- `TAP_DELTA_GRAMS`
- `TAP_MAX_DELTA_GRAMS`
- `TAP_RELEASE_DELTA_GRAMS`
- `TAP_SEQUENCE_GAP_MS`
- `TAP_SEQUENCE_MAX_MS`
- `TAP_REFRACTORY_MS`
- `TAP_COUNT_PROGRESS`
- `TAP_COUNT_PAUSE`
- `TAP_COUNT_MEETING`
- `TAP_COUNT_LIGHT_SHOW`
- `TAP_COUNT_NEXT_PROFILE`
- `TAP_COUNT_RESET_SESSION`
- `TAP_COUNT_END_DAY`

Se non vuoi usare i tap per i comandi, imposta in `UserConfig.h`:

```cpp
static const bool TAP_COMMANDS_ENABLED = false;
```

Per disattivare il sorso dinamico imposta `USE_DYNAMIC_SIP_SIZE` a `false` in `UserConfig.h`. In quel caso il firmware continua a contare i ml bevuti e mostra se il sorso e' troppo piccolo, ok o grande rispetto a `EXPECTED_SIP_ML`, ma non aumenta la quantita' consigliata e non recupera il ritardo chiedendo sorsi piu' grandi.

Se il tavolo vibra molto o vuoi rilevare pacche laterali, un piccolo IMU/giroscopio e' piu' affidabile della sola cella di carico.

## Alternative ai tap

La selezione via tap e' comoda per un puck chiuso, ma non e' l'unica opzione:

- Comandi seriali in `TEST_MODE`: ottimi per sviluppo e collaudo.
- Pulsante fisico sigillato IP67: soluzione piu' affidabile per uso quotidiano.
- Pulsante reed o Hall con magnete: nessun foro esposto, comando molto robusto.
- Selettore DIP o rotary interno: ideale per scegliere il profilo senza menu.
- BLE/Wi-Fi su ESP32: configurazione da telefono o pagina web locale.
- IMU/accelerometro: migliore dei tap su cella per pacche laterali e gesture.

Per il primo prototipo terrei i tap solo per test/azioni rare e userei auto-pausa + seriale. Se diventa un oggetto da usare ogni giorno, la scelta piu' pulita e' un pulsante sigillato o un sensore Hall.

## Stile luci

In `UserConfig.h` puoi scegliere:

```cpp
static const LightStyle LIGHT_STYLE = LightStyle::Calm;
```

- `LightStyle::Calm`: effetti sobri, bassi consumi, meno distrazione.
- `LightStyle::Vivid`: effetti piu' ricchi con comete, highlight e luminosita' piu' alta, mantenendo gli stessi colori semantici.

La modalita vivid non cambia la logica dei reminder: rende solo piu' scenografico il feedback.

## Modalita test

I comandi seriali sono sempre disponibili. In `UserConfig.h` puoi attivare una modalita test per avere debug piu' frequente o feedback hardware piu' evidente:

```cpp
static const TestMode TEST_MODE = TestMode::Full;
```

Valori disponibili:

- `TestMode::Off`: uso normale.
- `TestMode::Serial`: debug piu' frequente e comandi da Serial Monitor.
- `TestMode::Hardware`: feedback fisico piu' evidente su tap/comandi.
- `TestMode::Full`: seriale piu' hardware.

Comandi seriali disponibili:

- `h` o `help`: mostra l'elenco comandi.
- `s` o `status`: stampa stato corrente.
- `p`: mostra avanzamento.
- `a`: pausa/riprendi.
- `m`: meeting mode.
- `l`: gioco luci.
- `n`: prossimo profilo.
- `r`: reset sessione.
- `e`: fine giornata.
- `v`: impulso vibrazione.
- `g` o `grams`: stampa peso in grammi.
- `raw`: stampa letture raw HX711.
- `tare` o `zero`: tara e stampa `SCALE_ZERO_OFFSET`.
- `cal <grammi>`: calcola `SCALE_CALIBRATION_FACTOR` con un peso noto.
- `cal2 <grammi>`: calibrazione completa a due punti, calcola offset e scale.
- `cal2 reset`: annulla una calibrazione a due punti iniziata.
- `calauto`: calibrazione guidata a 3 punti, con feedback LED, attesa di 10 secondi e sampling automatico per ogni peso.

Da Telnet i comandi a singola lettera funzionano senza dover scrivere il nome completo, per esempio `l`, `g`, `s`, `t`, `v`, `r`.

## OTA e console remota

Su ESP32 puoi chiudere il puck e continuare a vedere log, inviare comandi e caricare firmware via rete.

In `UserConfig.h` configura:

```cpp
static const bool REMOTE_SERVICES_ENABLED = true;
static const char* WIFI_SSID = "broggiWifi";
static const char* WIFI_PASSWORD = "testtyyuu";
static const char* WIFI_FALLBACK_SSID = "Broggi_WiFi_2.4";
static const char* WIFI_FALLBACK_PASSWORD = "BroggiWifi11?";
static const char* OTA_HOSTNAME = "drinklight";
static const char* OTA_PASSWORD = "drinklight-ota";
static const uint16_t REMOTE_CONSOLE_PORT = 23;
```

Funzioni:

- OTA: dopo il primo caricamento via USB, Arduino IDE mostra una porta di rete per `drinklight`.
- Console remota: collegati via Telnet all'IP stampato in seriale, porta `23`.
- Wi-Fi: prova prima `WIFI_SSID`; se non si collega entro il timeout, prova `WIFI_FALLBACK_SSID`.
- I log della `SerialConsole` vengono duplicati su USB e Telnet.
- I comandi sono gli stessi della seriale: `status`, `grams`, `tare`, `cal2`, `light`, `reset`, ecc.
- Il logging completo su Telnet e' controllato da `REMOTE_CONSOLE_FULL_LOGGING` in `UserConfig.h`.

Note pratiche:

- Il primo upload deve essere via USB.
- OTA funziona solo se il dispositivo e il PC sono sulla stessa rete.
- Cambia `OTA_PASSWORD` prima di usare il puck fuori da una rete privata.
- Se il Wi-Fi non si collega entro `WIFI_CONNECT_TIMEOUT_SEC`, il firmware continua comunque in locale.

## Feedback dopo il sorso

Dopo ogni sorso registrato l'anello conferma anche la qualita' della bevuta:

- verde: sorso in linea con il riferimento;
- ambra parziale: sorso troppo piccolo;
- blu/verde dinamico: sorso abbondante.

Il riferimento e' dinamico solo se `USE_DYNAMIC_SIP_SIZE` e' attivo. Se e' spento, il riferimento resta `EXPECTED_SIP_ML`.

## Prima calibrazione

Nel file `HardwareConfig.h` modifica `SCALE_CALIBRATION_FACTOR` e, dopo la tara, `SCALE_ZERO_OFFSET`.

Procedura consigliata a due punti:

1. Avvia il puck.
2. Apri Serial Monitor a 115200 baud, newline attivo.
3. Appoggia il primo peso noto, per esempio niente peso: `0 g`.
4. Invia `cal2 0`.
5. Appoggia il secondo peso noto, per esempio `500 g`.
6. Invia `cal2 500`.
7. Copia `SCALE_ZERO_OFFSET=...` e `SCALE_CALIBRATION_FACTOR=...` in `HardwareConfig.h`.
8. Ricarica lo sketch.
9. Invia `grams` per verificare che il peso noto venga letto correttamente.

Puoi usare anche due pesi diversi da zero, per esempio `cal2 250` e poi `cal2 750`. Devono essere pesi reali e abbastanza distanti tra loro.

Procedura rapida alternativa:

1. Avvia senza pesi.
2. Invia `tare`.
3. Copia `SCALE_ZERO_OFFSET=...`.
4. Appoggia un peso noto.
5. Invia `cal <grammi>`.
6. Copia `SCALE_CALIBRATION_FACTOR=...`.

Il segno puo' essere positivo o negativo in base a come e' montata la cella.

## Idee funzionali utili

- Modalita focus: reminder discreto, solo luce progressiva e vibrazione rara.
- Modalita meeting: niente vibrazione, solo anello a bassa luminosita'. Implementata via 5 tap.
- Modalita catch-up: se `USE_DYNAMIC_SIP_SIZE` e' attivo e sei molto indietro, suggerisce sorsi piu' frequenti ma non enormi.
- Rilevamento refill: se il peso aumenta molto, aggiorna il peso base senza contarlo come bevuta.
- Fine sessione: riepilogo visivo e stop dei reminder. Implementata via 9 o piu' tap.
- Pausa pranzo: sospende il timer e non genera ritardo. Implementata in automatico se manca la borraccia a lungo.
- Profilo borraccia: soglie diverse per tazza, bicchiere o borraccia grande.

Vedi `docs/requirements.md` per una definizione requisiti piu' strutturata.
