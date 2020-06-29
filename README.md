# Progetto Sistemi Operativi e Laboratorio UniPi 2020

## Descrizione
Questo progetto simula il funzionamento di un supermercato, che sarebbe il processo principale, gestito da un Direttore, che è un thread, che si occupa di gestire K casse e C clienti (anch'essi thread). Il supermercato è pensato per permettere ai C clienti, che entrano contemporaneamente, di acquistare in tempo random un numero random di prodotti ciascuno, e di scegliere una cassa casuale in cui accodarsi. Il direttore è avvisato a intervalli regolari da ogni cassa sulla situazione di queste ultime, e se è il caso di aprire/chiudere una o più casse, è lui a farlo. I clienti che non acquistano vengono fatti uscire dal direttore. I cassieri servono i clienti in coda attendendo un tempo fisso random, diverso per ognuno di loro, e attendendo un tempo costante per ogni prodotto che varia a seconda del numero dei prodotti che sta passando. Il direttore inoltre, fa rientrare E clienti quando il supermercato conterrà C-E clienti. Il supermercato termina quando si ricevono dei segnali:<br>
<b>SIGQUIT:</b> Il supermercato termina immediatamente<br>
<b>SIGHUP:</b> Il direttore non fa entrare nuovi clienti e alla fine il supermercato termina.<br>
Al termine del programma viene scritto un file di log con le statistiche del supermercato.

## Come si usa
Si può creare l'eseguibile lanciando il makefile, dopodichè è possibile testare il funzionamento con un numero casuale di clienti, casse, ecc...<br>
Eventualmente si può lanciare il programma con un file di configurazione a piacere.
Il file deve essere nella stessa directory dell'eseguibile <i>"supermercato"</i>. Inoltre, il file deve avere il seguente formato:<br>

```
K=6
C=50
E=3
T=200
P=100
S1=2
S2=10
t_info=20
c_iniz=1
```
Dove K è il numero di casse totali, C è il numero di clienti totali, E è il numero di clienti che verranno fatti rientrare quando si arriva nella situazione di C = C-E, T è il tempo massimo che ciascun cliente può impiegare nell'acquistare i prodotti, P è il numero massimo di prodotti che il cliente può acquistare, S1 è il vincolo da rispettare per chiudere una cassa (se ci sono almeno S1 casse con al più 1 cliente), S2 è il vincolo da rispettare per aprire una nuova cassa (se c'è almeno una cassa con almeno S2 clienti in coda), t_info è l'intervallo con cui ciascuna cassa avvisa il direttore sullo stato della sua coda, c_iniz è il numero di casse aperte inizialmente.
<br>Per eseguirlo, dunque occorre:
```
./supermercato -c nomefile.txt
```
Per visualizzare poi l'output in questo modo occorre:

```
chmod +x analisi.sh
./analisi.sh
```
Altrimenti, per eseguire il test standard:
```
#aggiungere makefile
```