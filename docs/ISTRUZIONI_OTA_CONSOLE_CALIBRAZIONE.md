# DrinkLight - OTA, console remota e calibrazione

Questa guida risponde alle domande pratiche:

- Si collega a un Wi-Fi esistente?
- Come vedo la seriale quando il puck e' chiuso?
- Come carico nuovi sketch senza aprirlo?
- Come faccio la calibrazione della bilancia?

## Risposta breve

Si, il puck si collega a un Wi-Fi esistente.

Poi hai due modi per lavorare:

- OTA da Arduino IDE: carichi nuovi sketch via Wi-Fi.
- Console remota Telnet: vedi log e invii comandi come se fosse il Serial Monitor.

Il primo caricamento va fatto via USB. Dopo, se Wi-Fi e OTA funzionano, puoi chiudere il puck.

## Requisiti

Serve una scheda con Wi-Fi:

- ESP32.

Il firmware e' ESP32-only. Non mantenere selezionate schede ESP8266, Arduino UNO o Nano.

Il PC e il puck devono essere sulla stessa rete.

## 1. Configurare il Wi-Fi

Apri `UserConfig.h`.

Modifica questa sezione:

```cpp
static const bool REMOTE_SERVICES_ENABLED = true;
static const char* WIFI_SSID = "NomeDelTuoWifi";
static const char* WIFI_PASSWORD = "PasswordDelTuoWifi";
static const char* WIFI_FALLBACK_SSID = "NomeDelWifiDiRiserva";
static const char* WIFI_FALLBACK_PASSWORD = "PasswordWifiDiRiserva";
static const char* OTA_HOSTNAME = "drinklight";
static const char* OTA_PASSWORD = "scegli-una-password";
static const uint16_t REMOTE_CONSOLE_PORT = 23;
```

Esempio:

```cpp
static const char* WIFI_SSID = "CasaRossi";
static const char* WIFI_PASSWORD = "passwordwifi";
static const char* WIFI_FALLBACK_SSID = "CasaRossi_2.4";
static const char* WIFI_FALLBACK_PASSWORD = "passwordwifi2";
static const char* OTA_HOSTNAME = "drinklight";
static const char* OTA_PASSWORD = "mia-password-ota";
```

Importante:

- Il puck non crea una rete Wi-Fi.
- Si collega al router di casa/ufficio.
- Se la prima rete non si collega entro `WIFI_CONNECT_TIMEOUT_SEC`, prova la rete fallback.
- Usa preferibilmente Wi-Fi 2.4 GHz.
- Cambia `OTA_PASSWORD`.

## 2. Primo upload via USB

La prima volta devi usare il cavo USB.

1. Collega ESP32 via USB.
2. Apri `DrinkLight.ino` in Arduino IDE.
3. Seleziona una scheda ESP32 corretta, per esempio `ESP32 Dev Module` se usi una dev board generica.
4. Seleziona la porta USB.
5. Premi Upload.
6. Apri Serial Monitor a `115200 baud`.

Se il Wi-Fi funziona vedrai una riga simile:

```text
remote_wifi=connected ip=192.168.1.123 ota=drinklight telnet_port=23
```

Annota l'IP, per esempio:

```text
192.168.1.123
```

Se vedi:

```text
remote_wifi=failed
```

controlla:

- nome Wi-Fi;
- password;
- rete 2.4 GHz;
- distanza dal router;
- firewall/rete aziendale.

## 3. Come vedere la seriale da remoto

La seriale remota usa Telnet.

Se l'IP del puck e':

```text
192.168.1.123
```

da PowerShell puoi provare:

```powershell
telnet 192.168.1.123 23
```

Se Windows dice che `telnet` non esiste:

1. Apri "Attiva o disattiva funzionalita Windows".
2. Abilita "Client Telnet".
3. Riprova il comando.

In alternativa usa PuTTY:

1. Apri PuTTY.
2. Host: `192.168.1.123`
3. Port: `23`
4. Connection type: `Telnet`
5. Open.

Quando sei collegato, scrivi:

```text
help
```

Vedrai l'elenco comandi.

## 4. Comandi console utili

Questi comandi funzionano sia da USB Serial Monitor sia da Telnet:

```text
help        mostra aiuto
status      stato completo
grams       peso in grammi
raw         valori grezzi HX711
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
calauto     calibrazione guidata a 3 punti
```

I comandi brevi a singola lettera funzionano anche da Telnet:

```text
l    light
g    grams
s    status
t    tare
v    vib
r    reset
```

Nota importante:

Telnet mostra i log da quando ti colleghi. Non recupera i log gia' stampati prima della connessione.

Per vedere i log di boot completi, usa USB Serial Monitor.

La console Telnet riceve anche il logging periodico completo se in `UserConfig.h` e' attivo:

```cpp
static const bool REMOTE_CONSOLE_FULL_LOGGING = true;
```

Se noti rallentamenti, hai due opzioni:

```cpp
static const bool REMOTE_CONSOLE_FULL_LOGGING = false;
```

oppure aumentare l'intervallo del logging:

```cpp
static const uint16_t TEST_SERIAL_INTERVAL_MS = 500;
```

Con `REMOTE_CONSOLE_FULL_LOGGING = false`, Telnet riceve risposte ai comandi ed eventi importanti
come cambi stato, bottiglia appoggiata/rimossa, sorsi e sequenze tap, ma non il logging periodico.

## 5. Come caricare sketch via OTA

Dopo il primo upload via USB:

1. Lascia acceso il puck.
2. Aspetta che si colleghi al Wi-Fi.
3. Apri Arduino IDE.
4. Vai nella selezione porta.
5. Cerca una porta di rete chiamata `drinklight` o simile.
6. Selezionala.
7. Premi Upload.
8. Se richiesto, inserisci `OTA_PASSWORD`.

Se l'upload OTA riesce, non serve aprire il puck.

## 5B. Compilazione piu' rapida

Il firmware ESP32 compila piu' lentamente di uno sketch Arduino semplice perche' include Wi-Fi, OTA, Telnet, NeoPixel e HX711.

Impostazioni consigliate in Arduino IDE:

- Board: seleziona il modello ESP32 reale. Se e' una dev board generica, usa `ESP32 Dev Module`.
- Core Debug Level: `None`.
- PSRAM: `Disabled`, se la tua scheda non la usa.
- Partition Scheme: usa una partizione con OTA se vuoi aggiornamenti via rete. Non usare schemi `No OTA`.
- Upload Speed: alta, per esempio `921600`, solo se il cavo USB e la scheda sono stabili.

Abitudini utili:

- Non fare build pulite se non serve.
- Non cambiare board di continuo: cambia board = ricompilazione pesante.
- Tieni Arduino IDE aperto: le build successive sfruttano meglio la cache.
- Escludi da antivirus/Windows Defender queste cartelle:
  - cartella del repo `DrinkLight`;
  - `C:\Users\matteo.broggi\AppData\Local\Arduino15`;
  - `C:\Users\matteo.broggi\AppData\Local\arduino`;
  - `C:\Users\matteo.broggi\Documents\Arduino\libraries`.

Nota: rimuovere ESP8266/UNO dal codice semplifica il progetto, ma la parte lenta resta il core ESP32 con Wi-Fi/OTA.

## 6. Se la porta OTA non compare

Controlla in ordine:

1. Hai fatto almeno un primo upload USB?
2. Il puck stampa `remote_wifi=connected`?
3. PC e puck sono sulla stessa rete?
4. Arduino IDE e' stato riavviato dopo il primo avvio OTA?
5. Firewall o antivirus bloccano mDNS/Bonjour?
6. La rete Wi-Fi isola i dispositivi tra loro?

Su reti aziendali spesso mDNS e traffico tra client sono bloccati.

In quel caso la console Telnet potrebbe non funzionare e la porta OTA potrebbe non comparire.

## 7. Calibrazione consigliata della bilancia

La calibrazione si fa da console.

Puoi usare USB Serial Monitor oppure Telnet.

### Metodo migliore: `cal2`

Questo metodo calcola sia offset sia fattore scala.

1. Togli tutto dal puck.
2. Invia:

```text
cal2 0
```

3. Appoggia un peso noto, per esempio 500 g.
4. Invia:

```text
cal2 500
```

Il firmware stampera':

```text
SCALE_ZERO_OFFSET=...
SCALE_CALIBRATION_FACTOR=...
```

5. Copia quei valori in `HardwareConfig.h`:

```cpp
static const float SCALE_CALIBRATION_FACTOR = ...;
static const int32_t SCALE_ZERO_OFFSET = ...;
```

6. Ricarica lo sketch.
7. Verifica con:

```text
grams
```

Puoi usare anche due pesi diversi da zero:

```text
cal2 250
cal2 750
```

Devono essere pesi reali e abbastanza distanti.

### Metodo guidato: `calauto`

Questo metodo evita di dover mandare il comando esattamente nel momento giusto.
Usa 3 pesi noti e una regressione sui tre punti, quindi e' il metodo piu' comodo e robusto.

1. Invia:

```text
calauto
```

2. Quando chiede il primo peso, scrivi il valore in grammi, per esempio:

```text
0
```

3. Appoggia quel peso entro 10 secondi. Il LED diventa ambra durante l'assestamento e bianco durante il sampling.
4. Quando chiede il secondo peso, scrivi il valore in grammi, per esempio:

```text
500
```

5. Appoggia quel peso entro 10 secondi.
6. Quando chiede il terzo peso, scrivi un valore piu' alto, per esempio:

```text
1000
```

7. Appoggia quel peso entro 10 secondi. Il firmware campiona automaticamente e stampa `SCALE_ZERO_OFFSET` e `SCALE_CALIBRATION_FACTOR`.

Puoi annullare con:

```text
cancel
```

### Metodo rapido: `tare` + `cal`

1. Togli tutto dal puck.
2. Invia:

```text
tare
```

3. Copia `SCALE_ZERO_OFFSET`.
4. Appoggia un peso noto, per esempio 500 g.
5. Invia:

```text
cal 500
```

6. Copia `SCALE_CALIBRATION_FACTOR`.

## 8. Uso quotidiano

Il puck puo' restare sempre alimentato.

Non devi calibrarlo ogni mattina.

La logica e':

1. La bilancia usa `SCALE_ZERO_OFFSET` e `SCALE_CALIBRATION_FACTOR`.
2. Appoggi la bottiglia.
3. Il primo peso stabile diventa baseline.
4. Quando bevi e riappoggi la bottiglia, il calo di peso viene contato.
5. Se fai refill, il peso aumenta e la baseline viene aggiornata.
6. Dopo molte ore senza eventi, la sessione riparte da sola.

Quindi puoi arrivare con bottiglia vuota, mezza piena o piena.

## 9. Test LED

Invia:

```text
light
```

Il ring mostra:

- rosso;
- verde;
- blu;
- bianco;
- giallo;
- magenta;
- ciano;
- arancio.

Serve a capire se tutti i canali WS2812B funzionano.

## 10. Test vibrazione

Invia:

```text
vib
```

Il motorino deve fare un impulso breve.

## 11. Sicurezza

OTA e Telnet sono pensati per rete locale privata.

Non esporre il puck a Internet.

Da cambiare sempre:

```cpp
static const char* OTA_PASSWORD = "scegli-una-password";
```

Telnet non e' cifrato. Va bene per laboratorio/casa, non per reti non fidate.

## 12. Peso negativo

Se quando appoggi un peso leggi grammi negativi, non e' un problema meccanico grave: dipende dal verso della cella di carico o dal segno del fattore scala.

In `HardwareConfig.h` puoi correggerlo con:

```cpp
static const bool SCALE_INVERT_SIGN = true;
```

Se invece i pesi diventano negativi quando dovrebbero essere positivi, prova:

```cpp
static const bool SCALE_INVERT_SIGN = false;
```

Dopo aver cambiato questo valore, ricarica lo sketch e verifica con:

```text
grams
```
