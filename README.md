Informazioni Base: Gioco strategico
1vs1 (Human Player versus AI).
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
- Selezionata un unità si può effettuare una delle seguenti opzioni:
1) Muoversi e attaccare,
2) Attaccare e muoversi,
3) Attaccare e skippare il movimento,
4) Muoversi e skippare l'attacco
- Il player in turno può selezionare l'unità che vuole dividento a preferenza le azioni e l'ordine delle proprie unità.
- Il Turno termina al concludersi delle azioni disponibili di tutte le proprie unità. (IMPORTANTE: Premere "Skip Unit's Actions" se rimangono azioni che non si vogliono o non si possono compiere)
- L’avversario inizia il proprio turno come da punto 1 del Turno di Gioco.
Fine Partita
La partita finisce quando si verifica una delle seguenti condizioni:
1) Tutte le unità avversarie esauriscono i Punti Vita ➜ Vittoria del Player
2) Tutte le proprie unità esauriscono i Punti Vita ➜ Sconfitta del Player
Osservazioni e ulteriori specifiche
- Nella generazione degli ostacoli, tutte le celle sono comunque raggiungibili.
- Quando si seleziona la propria unità basterà premere sul bottone "Move" per evidenziare le caselle raggiungibili con il movimento, "Attack" per evidenziare le caselle attaccabili (ovvero in range e occupate da un unità nemica).
- Ogni unità potrà muoversi ed attaccare una sola volta a turno
- Sull'HUD sarà possibile visualizzare le caratteristiche di ogni unità su cui si clicca e le informazioni generiche della partita (Turni, punteggio, round giocati, storico mosse)
- Il giocatore che inizierà il turno verrà stabilito randomicamente all'inizio del gioco
- Ogni unità eliminata viene rimossa dalla griglia
- L’identificazione delle posizioni di ciascuna cella della griglia è formata da una lettera e da un numero (lettere A...Y dell’alfabeto inglese sull’asse orizzontale e numeri 1..25 sull’asse verticale). La cella più in basso a sinistra guardando la griglia è la A1
- Il movimento all'interno della griglia avviene attraverso il percorso più breve (composto solo da movimenti orizzontali e verticali, tenendo conto degli ostacoli)
- Le azioni dell’unità durante il turno di gioco sono mutualmente esclusive.
- Durante il turno, una volta confermata l’azione di una unità, questa non può più essere cambiata per quel turno.
- Una generica mossa da inserire nello storico delle mosse effettuate è composta da:
1) Identificativo del Player (HP per il player umano e AI per il player sfidante)
2) Identificativo dell'unità di gioco (S per lo Sniper e B per il Brawler)
3) Nel caso di una azione di movimento:
- Identificativo della cella di origine e della cella di destinazione
4) Nel caso di una azione di attacco:
- Identificativo della cella dell’avversario attaccato
- Danno apportato all’avversario
5) Nel caso del solo movimento o solo attacco, dovrà comparire solo la entry corrispondente.
Implementazioni extra:
- IA intelligente, prioritizza determinati bersagli, movimenti e mosse
- Contrattacco su Sniper;
Nel caso in cui lo Sniper effettui una mossa di attacco e:
- L'unità attaccata sia uno Sniper oppure
- L’unità attaccata sia un Brawler e si trovi a distanza 1 dallo stesso ->
- …lo Sniper riceverà un danno da contrattacco (random in un range da 1 a 3) che
verrà sottratto ai propri punti vita.
