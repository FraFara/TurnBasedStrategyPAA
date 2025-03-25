Informazioni Base: Gioco strategico
● 1vs1 (Human Player versus AI).
Mappa di gioco
● Dimensione Mappa: Griglia di 25 celle x 25 celle -> dimensioni editabili dalla classe blueprint BP_TBSGameMode
● Griglia di celle quadrate vista dall'alto
● Presenza di ostacoli: celle di colore diverso non percorribili -> percentuale configurabile dalla classe blueprint BBP_TBSGameMode
Tipologia Unità di Gioco:
Sniper:
● Movimento: Max 3 celle
● Tipo di Attacco: Attacco a distanza
● Range Attacco: Max 10
● Danno: val. min 4 ↔ val. max 8
● Vita: 20
Brawler:
● Movimento: Max 6 celle
● Tipo di Attacco: Attacco a corto raggio
● Range Attacco: 1
● Danno: val. min 1↔ val. max 6
● Vita: 40
Fasi Partita:
1) Scelta della difficoltà -> Easy (Random AI), Hard (Smart AI)
2) Ad entrambe le parti in gioco vengono assegnate 2 Unità di gioco, una per tipo.
3) Subito dopo avviene un lancio di una moneta per stabilire il primo giocatore che inizierà a piazzare le unità cliccando sulla cella dove vuole spawnarle.
4) Finita la fase PLACEMENT inizierà il gioco vero e proprio, in cui ogni giocatore potrà muovere ed attaccare con le proprie unità. Se non volesse compiere una azione (o non potesse attaccare) Il tasto SKIP permette di skippare le azioni rimanenti disponibili per l'unità selezionata
5) Se un unità viene selezzionata viene segnalato dall'HUD in basso a sinistra, mentre sulla destra compariranno le statistiche e la vita dell'ultima unità cliccata (alleata o nemica)
6) Quando una delle due squadre rimane senza unità si procede al game over -> premendo il bottone reset si inizierà un nuovo round mantenendo il punteggio (se premuto prima della fine del round il punteggio rimarrà invariato e ricomincerà il round.

Turno di Gioco
Selezionata un unità si può effettuare una delle seguenti opzioni:
a. Eseguire prima un movimento e poi attaccare (se il range di attacco lo
consente)
b. Attaccare senza effettuare il movimento.
c. Eseguire un movimento senza attaccare.
Nel caso che una unità non possa effettuare un movimento, può solo effettuare un
attacco. In generale, non dovrebbe essere mai possibile che l’unità non possa
eseguire almeno un’azione.
3. Il player in turno seleziona la sua seconda unità ed esegue la medesima azione
descritta nel punto 2.
4. il Turno termina al concludersi delle azioni disponibili di tutte le proprie unità.
5. L’avversario inizia il proprio turno come da punto 1 del Turno di Gioco.
Fine Partita
La partita finisce quando si verifica una delle seguenti condizioni:
● Tutte le unità avversarie esauriscono i Punti Vita ➜ Vittoria del Player
● Tutte le proprie unità esauriscono i Punti Vita ➜ Sconfitta del Player
Osservazioni e ulteriori specifiche
● Nella generazione degli ostacoli, tutte le celle devono essere comunque raggiungibili
(no isole/sezioni di mappa irraggiungibili).
● Quando si clicca sulla propria unità per selezionarla, deve essere mostrato sulla
griglia il range possibile di movimento. Con un ulteriore click dovrà essere nascosto.
○ Nel caso della AI, questa funzionalità è automatica: quando effettua la propria
mossa, si evidenzia sulla griglia il movimento e/o il range di attacco.
● Nel caso venga effettuato un movimento su un'unità, non deve essere possibile
muoverla nuovamente nello stesso turno.
● Dovrà essere possibile visualizzare lo stato delle unità per capire quando la partita
termina (stampate nel log o in un widget, a lato della griglia oppure sulle unità stesse)
● Dovrà essere mostrato il turno di gioco (Player oppure AI)
● Il lancio della moneta può essere realistico (moneta che viene lanciata in aria e che
si ferma su di un piano) oppure simulata (ad esempio, viene premuto un tasto e
compare la scritta di chi inizia il turno)
● Le unità che esauriscono i Punti Vita dovranno essere rimosse dalla griglia
● L’identificazione delle posizioni di ciascuna cella della griglia è formata da una lettera
e da un numero (lettere A...Y dell’alfabeto inglese sull’asse orizzontale e numeri
1..25 sull’asse verticale). La cella più in basso a sinistra guardando la griglia è la A1
● Il movimento delle unità nella griglia dovrà avvenire seguendo il percorso minimo tra
il punto di partenza e quello di arrivo sulla griglia (no teletrasporto, no linea retta tra i
due punti)
● Le azioni dell’unità durante il turno di gioco sono mutualmente esclusive.
● Durante il turno, una volta confermata l’azione di una unità, questa non può più
essere cambiata per quel turno.
● Una generica mossa da inserire nello storico delle mosse effettuate è composta da:
○ Identificativo del Player (HP per il player umano e AI per il player sfidante)
○ Identificativo dell'unità di gioco (S per lo Sniper e B per il Brawler)
○ Nel caso di una azione di movimento:
■ Identificativo della cella di origine e della cella di destinazione
○ Nel caso di una azione di attacco:
■ Identificativo della cella dell’avversario attaccato
■ Danno apportato all’avversario
○ Esempio: Supponiamo che il giocatore muova il proprio Sniper e poi decida di
attaccare con lo stesso. Nello storico delle mosse dovranno apparire le
seguenti righe:
■ HP: S B4 -> D6
■ HP: S G8 7
○ Nel caso del solo movimento o solo attacco, dovrà comparire solo la entry
corrispondente.
● Nel caso dell’implementazione dell’algoritmo A*, l’IA si deve muovere verso una unità
nemica (sempre nel range di movimento).
● Specifiche Danno da Contrattacco
○ Nel caso in cui lo Sniper effettui una mossa di attacco e:
■ L'unità attaccata sia uno Sniper oppure
■ L’unità attaccata sia un Brawler e si trovi a distanza 1 dallo stesso
○ …lo Sniper riceverà un danno da contrattacco (random in un range) che
verrà sottratto ai propri punti vita.
○ Range danno da contrattacco: min 1 max 3
