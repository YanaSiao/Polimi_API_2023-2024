# Polimi_API_2023-2024
Progetto realizzato per il corso "PROVA FINALE (PROGETTO DI ALGORITMI E PRINCIPI DELL'INFORMATICA)" nell'anno accademico 2023/2024.

##Descrizione del progetto
Il progetto riguarda lo sviluppo di un software per simulare il sistema di gestione degli ordini di una pasticceria industriale. La simulazione avviene a tempo discreto, avanzando di un'unità temporale per ogni comando eseguito, e gestisce vari elementi chiave come il magazzino degli ingredienti, le ricette disponibili e gli ordini dei clienti. Il sistema deve gestire operazioni come l'aggiunta e la rimozione di ricette, il rifornimento degli ingredienti e l'evasione degli ordini. La simulazione termina quando viene elaborato l'ultimo comando del file di input.

- Gli ingredienti sono immagazzinati in lotti con una quantità specifica e una data di scadenza.
- Le ricette sono definite da un nome e da una lista di ingredienti con quantità precise
- La preparazione degli ordini avviene immediatamente se le scorte sono sufficienti; in caso contrario, gli ordini vengono messi in attesa fino al rifornimento del magazzino.
- Il corriere ritira periodicamente gli ordini pronti, rispettando un limite di capienza e seguendo una priorità basata sull'ordine di arrivo e sul peso dei prodotti.

##Analisi della soluzione 

### Struttura dei dati e funzionamento del programma

- **ricetta** è una struct che rappresenta una singola ricetta. 
- **ricette** è una tabella hash che memorizza tutte le ricette esistenti. Ogni bucket della tabella è un array ordinato per nome della ricetta, poiché i nomi sono unici. Questa organizzazione consente di effettuare ricerche rapide tramite binary search.
- **order** è una struct che rappresenta un ordine. Gli ordini vengono memorizzati in un array poiché i nuovi ordini vengono sempre aggiunti in fondo, mentre quelli più vecchi vengono analizzati ed estratti solo dall'inizio, rispettando l'ordine di inserimento.
- **magazzino** è una tabella hash che contiene nodi di tipo lotto. 
- **lotto** è una struct che memorizza le informazioni relative a un particolare ingrediente. Ogni nodo lotto conserva i dettagli di tutti i lotti di un determinato ingrediente. Se esistono più lotti dello stesso ingrediente con date di scadenza diverse, il sistema può individuarli rapidamente. I bucket della tabella hash sono ordinati lessicograficamente per nome dell’ingrediente.
- 
### Funzionamento del programma

Il **main** alloca la memoria necessaria, legge i dati relativi agli ordini e ai rifornimenti, chiama il metodo distinguish_request, rileva la fine dell'input e libera la memoria allocata.

Il metodo **distinguish_request** analizza il comando ricevuto e segue uno dei quattro scenari possibili, chiamando il metodo corrispondente.

Il metodo **rifornimento** memorizza gli ingredienti ricevuti e verifica immediatamente se è possibile evadere un ordine dalla lista degli ordini in attesa. Inoltre, ogni ordine in attesa registra il nome dell'ingrediente mancante **missing_ing** e la quantita **missing_q**, che viene controllato prima di tutti gli altri. Se tale ingrediente non è stato aggiunto nel rifornimento, l'ordine non viene verificato, poiché si sa già che l'ingrediente precedentemente mancante è ancora indisponibile.

[Specifica](https://github.com/YanaSiao/Polimi_API_2023-2024/blob/main/specifica%202024.pdf)

##Suggerimenti per altri studenti

###Pianificazione prima di scrivere il codice
Prima di cominciare a scrivere il codice, è fondamentale pianificare le strutture dati su carta. Dedica un po' di tempo a disegnare uno schema di base delle strutture e a capire come dovranno essere collegate tra loro. Questo ti darà una visione chiara dell'architettura del progetto e ti aiuterà a prevenire problemi in fase di implementazione.

### Comprendere il flusso e le interazioni
La seconda fase consiste nel comprendere come interagiranno tra loro le varie componenti del programma. Hai bisogno di una timeline o di altre variabili globali? Sarà necessario riutilizzare un pezzo di codice? Se sì, scrivi un metodo che svolga quel compito e richiamalo ogni volta che ne avrai bisogno. Se due metodi risultano molto simili ma restituiscono aspetti diversi di una struttura, valuta la possibilità di unificarli e restituire una struttura contenente tutte le informazioni necessarie, estraendo i dati utili al momento. Questo approccio aiuta a mantenere il codice pulito, riutilizzabile e facilmente espandibile in futuro.

###Scegliere le strutture dati giuste
Analizza bene quali strutture dati sono veloci e non consumano troppa memoria, e inizia a lavorare con quelle. Nel mio caso, inizialmente avevo implementato delle liste ordinate, ma passando a hashmap ho risparmiato circa 10 secondi di tempo di esecuzione complessivo. Sperimenta e scegli le soluzioni più adatte alla tua situazione specifica.

###Attenzione alle perdite di memoria
Non sottovalutare le perdite di memoria. Sono il "demonio" che consuma tutte le risorse del tuo programma. Usa Valgrind per analizzare il tuo codice, anche se non l'hai mai usato prima e ti sembra scomodo. È meglio imparare a usarlo ora, che trovarsi in difficoltà nell'ultima settimana prima della scadenza del progetto.

###Non avere paura di chiedere aiuto
Se ti senti bloccato o hai delle difficoltà, non esitare a chiedere aiuto.

Good luck, and may the force be with you!
