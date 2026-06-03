# DrinkLight - mappa luci e animazioni

Questa pagina descrive il significato visivo dell'anello LED.

## Priorita

Se piu' effetti sono attivi insieme, prevale il primo della lista:

1. calibrazione guidata;
2. conferma sequenza tap in modalita test hardware/full;
3. feedback comando;
4. stato idratazione.

## Stati idratazione

| Stato | Colore | Animazione | Significato |
| --- | --- | --- | --- |
| `BottleMissing` | blu scuro | un LED ruota lentamente | borraccia assente |
| `Quiet` | spento | nessuna | ritmo corretto |
| `Ramp` | blu | riempimento progressivo; highlight mobile in stile `Vivid` | si avvicina il prossimo sorso |
| `Due` | verde acqua | anello pieno pulsante; highlight mobile in stile `Vivid` | e' il momento di bere |
| `Overdue` | rosso | lampeggio dell'anello; highlight bianco mobile in stile `Vivid` | sorso in ritardo |
| `PausedManual` / `PausedAuto` | giallo | due LED opposti respirano lentamente | reminder sospesi |
| `Meeting` | indaco | due LED discreti o riempimento attenuato | meeting mode, feedback ridotto |
| `RefillDetected` | viola | doppia onda speculare veloce | refill rilevato |
| `Complete` | verde chiaro | anello pieno o cometa in stile `Vivid` | target raggiunto |
| `DayComplete` | verde scuro | avanzamento finale | giornata chiusa |

## Feedback dopo un sorso

| Deduzione | Colore | Animazione | Significato |
| --- | --- | --- | --- |
| sorso piccolo | arancione | porzione di anello proporzionale ai ml bevuti rispetto al riferimento | sorso contato ma sotto il valore consigliato |
| sorso corretto | verde brillante | doppia onda speculare o sparkle in stile `Calm` | sorso in linea |
| sorso abbondante | lime | cometa o sparkle in stile `Calm` | sorso sopra il riferimento |

## Feedback comandi

| Comando | Colore | Animazione |
| --- | --- | --- |
| `progress` / 3 tap | verde | mostra avanzamento con highlight mobile |
| `pause` / 4 tap | arancione | due LED opposti |
| `meeting` / 5 tap | blu | mezzo anello |
| `light` / 6 tap | multicolore | gioco luci completo |
| `profile` / 7 tap | viola | avanzamento proporzionale al profilo selezionato |
| `reset` / 8 tap | bianco | cometa bianca con coda grigia |
| `end` / 9+ tap | verde | avanzamento finale |

## Conferma sequenza tap

Con `TEST_MODE = TestMode::Hardware` o `TestMode::Full`, una sequenza riconosciuta genera lampeggi viola.
Con `TAP_ONLY_WHEN_BOTTLE_MISSING = true`, le gesture vanno eseguite sul puck vuoto dopo aver tolto la borraccia.

Il numero di lampeggi corrisponde al numero di tap rilevati:

- 3 tap: 3 lampeggi viola;
- 5 tap: 5 lampeggi viola;
- 9 tap: 9 lampeggi viola.

La sequenza viene confermata solo dopo una pausa di `TAP_SEQUENCE_GAP_MS` dall'ultimo tap.
Per uscire dalla pausa manuale, togli la borraccia e ripeti 4 tap.

## Calibrazione guidata

| Fase | Colore | Animazione |
| --- | --- | --- |
| richiesta peso | blu | avanzamento della procedura con LED mobile |
| attesa assestamento | ambra | countdown progressivo |
| sampling HX711 | bianco | cometa |
| calibrazione riuscita | verde | anello pieno |
| errore o annullamento | rosso | lampeggio rapido |

## Gioco luci

Il comando `light` o la sequenza da 6 tap avvia una sequenza di circa 8 secondi:

1. arcobaleno rotante;
2. doppia onda magenta;
3. pattern mobile bianco, rosso, lime e blu;
4. flash finale bianco pulsante.

## Stili

- `LightStyle::Calm`: feedback piu' sobri, con sparkle e riempimenti semplici.
- `LightStyle::Vivid`: highlight mobili, onde e comete piu' visibili.
