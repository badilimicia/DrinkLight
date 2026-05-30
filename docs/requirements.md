# DrinkLight - definizione requisiti

## Obiettivo

Realizzare un puck smart da scrivania su cui appoggiare una borraccia. Il dispositivo deve stimare quanta acqua viene bevuta durante una sessione di lavoro e ricordare in modo progressivo quando bere, usando luce e vibrazione senza richiedere interazione continua.

## Utente target

Persona che lavora al computer per molte ore, tende a dimenticarsi di bere e vuole un promemoria fisico non invasivo, visibile in periferia e comprensibile senza app.

## Requisiti funzionali

### RF-01 Misura peso

Il sistema deve leggere il peso tramite cella di carico e HX711, filtrare il rumore e distinguere tra:

- puck vuoto;
- borraccia presente;
- peso instabile;
- peso stabile.

### RF-02 Riconoscimento sorso

Il sistema deve registrare un sorso quando:

- la borraccia era presente con peso stabile;
- viene sollevata o il peso cambia;
- viene riappoggiata;
- il nuovo peso stabile e' inferiore al precedente;
- la differenza e' dentro soglie configurabili.

Differenze troppo piccole devono essere ignorate come rumore. Differenze troppo grandi devono essere trattate come anomalia, refill o cambio borraccia.

Il sistema non deve richiedere una calibrazione manuale della quantita' iniziale nella borraccia. Il primo peso stabile della bottiglia deve diventare automaticamente la baseline, indipendentemente dal fatto che sia vuota, mezza piena o piena.

### RF-02B Baseline, refill e cambio bottiglia

Il sistema deve distinguere tra:

- tara bilancia: puck vuoto, necessaria per misurare correttamente;
- baseline bottiglia: primo peso stabile della bottiglia, automatica;
- refill: aumento significativo del peso, aggiorna la baseline;
- cambio/anomalia: diminuzione troppo grande per essere un sorso, aggiorna la baseline senza contare bevuta.

### RF-03 Piano idratazione

Il sistema deve distribuire un target di acqua lungo una sessione:

- target sessione, esempio 1000 ml;
- durata sessione, esempio 4 ore;
- sorso atteso, esempio 80 ml.

In modalita dinamica, il reminder deve basarsi sia sul tempo trascorso sia sull'acqua effettivamente bevuta.

Se l'utente beve prima del dovuto in modalita dinamica, il sistema deve considerare quei ml come credito e ritardare i reminder successivi. Il sorso dinamico deve essere disattivabile: in modalita statica il firmware conta comunque i ml e classifica il sorso, ma il reminder lavora solo sulla cadenza temporale basata sul sorso atteso configurato.

### RF-04 Effetti luminosi

L'anello LED deve comunicare lo stato senza testo:

- vuoto: piccolo LED blu di localizzazione;
- in linea: spento;
- avvicinamento al prossimo sorso: accensione progressiva blu;
- sorso dovuto: pulsazione verde/acqua;
- ritardo: lampeggio rosso/arancio;
- sorso registrato: breve animazione positiva verde;
- sorso insufficiente: feedback ambra breve;
- sorso abbondante: feedback blu/verde breve;
- pausa: luce ambra molto lenta;
- meeting mode: luce blu molto bassa, senza vibrazione;
- refill: breve onda blu;
- target raggiunto: anello verde pieno.

### RF-05 Vibrazione

Il motorino deve vibrare solo quando l'utente e' in ritardo, con impulsi brevi e distanziati. Deve essere disattivabile via configurazione.

### RF-06 Debug seriale

Durante sviluppo e calibrazione il firmware deve stampare su Serial Monitor:

- peso letto;
- stabilita';
- presenza borraccia;
- ml consumati;
- ml consigliati per il prossimo sorso;
- bilancio in ml rispetto al ritmo previsto;
- stato reminder.

### RF-07 Interazioni senza touch esposto

Il sistema deve supportare comandi semplici tramite tocchi/spinte rapide rilevati dalla cella di carico:

- 3 tap: mostra avanzamento;
- 4 tap: pausa o riprendi;
- 5 tap: meeting mode;
- 6 tap: gioco luci;
- 7 tap: cambia profilo sessione;
- 8 tap: reset sessione;
- 9 o piu' tap: fine giornata.

Questi comandi devono essere opzionali e tarabili per evitare falsi positivi.

I comandi via tap devono poter essere disattivati da configurazione. Il progetto deve prevedere alternative per un uso piu' affidabile: comandi seriali in test, pulsante sigillato, sensore Hall/reed, IMU o configurazione BLE/Wi-Fi.

### RF-08 Pausa e fine giornata

Il sistema deve sospendere il calcolo del ritardo quando:

- l'utente mette manualmente in pausa;
- la borraccia manca per un tempo prolungato;
- la sessione viene chiusa a fine giornata.

La pausa automatica deve riprendere quando la borraccia torna presente.

### RF-08B Riavvio sessione trasparente

Il puck puo' restare sempre alimentato. Il sistema non deve richiedere ricalibrazione giornaliera della bilancia: deve riavviare solo il contatore della sessione.

La nuova sessione deve partire quando:

- passa una soglia configurabile di ore senza eventi utili;
- l'utente forza un reset manuale;
- il firmware parte da zero dopo accensione/reset.

Il riavvio sessione deve azzerare i consumi e cancellare la baseline bottiglia, ma non deve modificare offset o calibrazione HX711.

### RF-09 Modalita test

Il sistema deve supportare una modalita test attivabile da configurazione per:

- stampare piu' spesso stato, peso, profilo e comandi su seriale;
- provare i comandi da Serial Monitor senza tap fisici;
- verificare tap, LED e vibrazione con feedback hardware piu' evidente.

I comandi seriali minimi devono coprire avanzamento, pausa, meeting, gioco luci, cambio profilo, reset, fine giornata e tara.

La modalita seriale deve includere anche un ciclo di calibrazione completo:

- `tare`/`zero`: calcola l'offset a puck vuoto e stampa il valore da copiare in configurazione;
- `cal <grammi>`: con peso noto appoggiato, calcola il fattore scala e stampa il valore da copiare in configurazione;
- `cal2 <grammi>`: registra due punti di calibrazione e calcola offset piu' fattore scala;
- `grams`: stampa il peso convertito;
- `raw`: stampa valori grezzi per debug HX711.

### RF-10 OTA e console remota

Su ESP32 il sistema deve supportare:

- connessione Wi-Fi configurabile;
- aggiornamento firmware OTA dopo il primo caricamento USB;
- console remota Telnet per vedere log e inviare gli stessi comandi seriali;
- fallback locale se il Wi-Fi non e' disponibile.

Il firmware e' ESP32-only: compilare per schede non ESP32 deve fallire esplicitamente.

## Requisiti non funzionali

- Arduino IDE: il progetto deve restare caricabile senza PlatformIO.
- Loop non bloccante: niente `delay()` nel ciclo principale.
- Configurazione separata tra `UserConfig.h` e `HardwareConfig.h`.
- Gestione modulare dei componenti: seriale, bilancia, tracking idratazione, LED, tap e vibrazione devono restare in file separati.
- Effetti luminosi leggibili anche a bassa luminosita'.
- Soglie modificabili senza riscrivere la logica.
- Hardware sicuro: motorino pilotato con transistor/MOSFET, non direttamente dal pin.

## Stati principali

- `BottleMissing`: borraccia assente o peso sotto soglia.
- `PausedManual`: pausa attivata dall'utente.
- `PausedAuto`: pausa attivata per assenza prolungata della borraccia.
- `Meeting`: reminder attenuati e vibrazione disattivata.
- `RefillDetected`: aumento peso interpretato come refill.
- `Quiet`: utente in linea con il piano.
- `Ramp`: si avvicina il prossimo sorso.
- `Due`: e' il momento di bere.
- `Overdue`: l'utente e' in ritardo.
- `JustDrank`: sorso appena registrato.
- `Complete`: target raggiunto.
- `DayComplete`: sessione chiusa.

## Illuminazioni consigliate

### Reminder progressivo

L'anello si riempie in senso orario negli ultimi minuti prima del sorso previsto. Questo e' utile perche' non interrompe subito l'utente ma rende visibile che il momento si avvicina.

La resa visiva deve supportare almeno due stili:

- `Calm`: sobrio, discreto, adatto alla scrivania.
- `Vivid`: piu' luminoso e animato, ma con gli stessi colori semantici per non perdere leggibilita'.

### Conferma sorso

Quando un sorso viene rilevato, il feedback deve dire due cose: "ha contato" e "era abbastanza".

- Verde dinamico: sorso corretto.
- Ambra parziale: sorso troppo piccolo.
- Blu/verde dinamico: sorso abbondante, utile se eri in ritardo.

Con sorso dinamico disattivato, questi feedback devono essere calcolati rispetto al valore fisso `EXPECTED_SIP_ML`, senza modificare la cadenza dei reminder.

### Ritardo

Il lampeggio rosso/arancio deve essere evidente ma non continuo. Dopo un breve periodo di grazia puo' partire anche la vibrazione.

### Target raggiunto

Anello verde pieno o animazione breve. Dopo qualche secondo puo' tornare spento per non consumare energia.

## Funzioni future

- Salvataggio sessione su memoria interna.
- BLE o Wi-Fi per dashboard giornaliera.
- Sincronizzazione orario reale e fasce di lavoro.
- Sensore batteria se il puck diventa portatile.
- Calibrazione guidata via seriale.
- EEPROM/Preferences per salvare target e calibrazione.
- IMU/giroscopio per pacche laterali piu' robuste rispetto ai tap sulla cella di carico.
- Salvataggio del profilo attivo in memoria.
- Storico giornaliero minimo: totale bevuto e numero di pause.

## Criteri di accettazione MVP

- Con borraccia ferma, il peso non deve oscillare oltre pochi grammi dopo stabilizzazione.
- Un sorso reale di almeno 20 ml deve essere contato entro pochi secondi dal riappoggio.
- Un piccolo spostamento della borraccia non deve essere contato come bevuta.
- Se l'utente non beve, l'anello deve passare da spento a progressivo, poi a pulsante, poi a ritardo.
- In ritardo, la vibrazione deve essere breve e ripetuta a intervalli, non continua.
- Un tap deve sospendere i reminder senza richiedere pulsanti touch esposti.
- Una pausa lunga non deve generare ritardo artificiale.
