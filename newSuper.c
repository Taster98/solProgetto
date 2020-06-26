#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
/*QUI DEFINISCO LE VARIE COSTANTI DEL PROGRAMMA*/
//tempo minimo acquisto per i clienti
#define T_MIN_ACQUISTI 10
//tempo minimo fisso per i cassieri
#define T_MIN_CASS 20
//tempo massimo fisso per i cassieri
#define T_MAX_CASS 80
//Includo header ad-hoc
//file aggiuntivi
#include "./headers/cassiere.h"
#include "./headers/client.h"
#include "./errors/error.h"
#include "./headers/coda.h"
/*STRUTTURA DEL FILE DI CONFIGURAZIONE, CHE CONSERVA TUTTI I DATI*/
typedef struct config{
    int K; //casse max apribili
    int C; //clienti totali
    int E; //chi esce/entra
    int T; //tempo max per gli acquisti cliente
    int P; //numero max prodotti per clienti
    int S; //tempo per un cassiere per passare un prodotto
    int S1; //Se S1=N e ci sono N casse con C<=1 chiudo una cassa
    int S2; //Se S2=M e ci sono M clienti in una cassa, se ne apre una nuova
    //configurazioni extra
    int tempo_info; //tempo per un cassiere per informare il direttore
    int casse_iniziali; //casse aperte all'inizio
    int num_clienti; //numero clienti serviti
    int num_prodotti; //numero prodotti acquistati
}config;

//VARIABILI GLOBALI
//Variabile globale del file di configurazione
config cfg;
//Variabile globale per il file di log
FILE *flog;
//Variabili globali per gestire i segnali SIGHUP e SIGQUIT
volatile sig_atomic_t sighup = 0;
volatile sig_atomic_t sigquit = 0;

//Coda del direttore
coda *codaDirettore;
//Mutex per accedere alla coda del direttore
pthread_mutex_t mutexCodaDirettore = PTHREAD_MUTEX_INITIALIZER;
//Variabile di condizione per mettere il direttore in attesa di coda piena
pthread_cond_t condCodaDirettore = PTHREAD_COND_INITIALIZER;

//Array di code dei cassieri (saranno cfg.K)
coda **codeCassieri;
//Array di mutex per le code dei cassieri
pthread_mutex_t *mutexCodeCassieri;
//Array di variabili di condizione per le code dei cassieri
pthread_cond_t *condCodeCassieri;

//Array di cassieri
cassiere *casse;
//Mutex gestione array
pthread_mutex_t mutexCasse = PTHREAD_MUTEX_INITIALIZER;

//Mutex gestione logFile
pthread_mutex_t mutexLogFile = PTHREAD_MUTEX_INITIALIZER;


int *arrayInformazioni;
pthread_mutex_t mutexInformazioni = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condInformazioni = PTHREAD_COND_INITIALIZER;


//Array di casse chiuse
int *casseChiuse;
pthread_mutex_t *mutexCasseChiuse;
pthread_cond_t *condCasseChiuse;

//Funzione per gestire i segnali (HANDLER) - PROTOTIPO
void handler(int signum);

//FUNZIONI DI INIZIALIZZAZIONE - PROTOTIPI
//Funzione che inizializza il file di configurazione
void inizializzaConfig();

//Inizializzazione strutture varie (coda direttore, code cassieri, mutex e cond...)
void initAll();
//Pulizia strutture varie
void cleanAll();

//FUNZIONI DEI THREAD
//Thread Direttore
void *Direttore(void *arg);
//Thread di appoggio del Direttore per gestire le Casse
void *DirettoreCasse(void *arg);
//Thread di appoggio del Direttore per gestire i Clienti
void *DirettoreClienti(void *arg);
//Thread delle Casse
void *Casse(void *arg);
//Thread dei Clienti 
void *Clienti(void *arg);
//Thread per il Timer che ogni cfg.S millisecondi avvisa il direttore sullo stato delle casse
void *TimerCasse(void *arg);


//FUNZIONI AGGIUNTIVE - PROTOTIPI
int contaCasseAperte(cassiere *c);
cassiere *casseAperte(cassiere *c, int count);
//funzione per stampare più facilmente (Mette anche a capo da solo)
void print(char *s);
int *queueDimensions();
int esseUno();
int esseDue();
int informazioniRicevute();
void ritornaMenoUno();
//THREAD PRINCIPALE - MAIN (Supermercato)
int main(int argc, char *argv[]){
    /*Il main coincide col processo Supermercato,
    quindi l'unica cosa che deve fare è  inizializzare
    i segnali e poi avviare il thread Direttore e 
    attendere che termini:*/

    //INIZIALIZZO HANDLER E SEGNALI
    struct sigaction s;
    memset(&s,0,sizeof(s));
    s.sa_handler=handler;

    /* (?)
    sigset_t handlerMask;
    ec_meno1(sigemptyset(&handlerMask), "Errore nello svuotare la handlerMask\n");
    ec_meno1(sigaddset(&handlerMask, SIGHUP), "Errore nell'aggiunta di SIGHUP\n");
    ec_meno1(sigaddset(&handlerMask, SIGQUIT), "Errore nell'aggiunta di SIGQUIT\n");
    */

    ec_meno1(sigaction(SIGHUP,&s,NULL),"Errore segnale di SIGHUP");
    ec_meno1(sigaction(SIGQUIT,&s,NULL),"Errore segnale di SIGQUIT");
    //Inizializzazione delle varie strutture:
    inizializzaConfig();
    initAll();
    //Creo il direttore:
    pthread_t id_direttore = 1;
    ec_zero(pthread_create(&id_direttore,NULL,Direttore,NULL),"Errore creazione Thread Direttore");
    print("SUPERMERCATO APERTO");
    //Attendo che il direttore termini:
    ec_zero(pthread_join(id_direttore,NULL),"Errore join Direttore");
    print("DIRETTORE TERMINATO");
    //Pulizia heap
    cleanAll();
    print("SUPERMERCATO CHIUSO");
    return 0;
}

//FUNZIONI DEI THREAD

/*Thread Direttore: Si occupa solamente di avviare gli altri thread e gestire i segnali.
In pratica, finchè i segnali sono 0 (SIGQUIT e SIGHUP), allora dopo aver lanciato i vari 
thread non fa nulla */
void *Direttore(void *arg){
    print("DIRETTORE AVVIATO");
    //Avvio i sottothread del direttore
    pthread_t id_direttore_casse = 2;
    pthread_t id_direttore_clienti = 3;
    int num = 7;
    ec_meno1(pthread_create(&id_direttore_casse,NULL,DirettoreCasse,&num),"Errore creazione thread Direttore Casse");
    print("DIRETTORE CASSE AVVIATO");
    ec_meno1(pthread_create(&id_direttore_clienti,NULL,DirettoreClienti,NULL),"Errore creazione thread Direttore Clienti");
    print("DIRETTORE CLIENTI AVVIATO");
    
    //Join dei sottothread direttore
    ec_meno1(pthread_join(id_direttore_clienti,NULL),"Errore join thread Direttore Clienti");
    print("DIRETTORE CLIENTI TERMINATO");
    ec_meno1(pthread_join(id_direttore_casse,NULL),"Errore join thread Direttore Casse");
    print("DIRETTORE CASSE TERMINATO");
    //Dovrò gestire i segnali
    //Esco
    pthread_exit(NULL);
}

/*Thread ausiliario Direttore Casse: Si occupa di aprire/chiudere le casse in base alle
informazioni che riceve da ogni thread TimerCassa ogni tempo fissato (cfg.S). Si occupa
anche dell'avvio delle cfg.K Casse e attende quindi il loro termine per terminare.*/

/*Il direttore sta in attesa che gli arrivino le notifiche da tutte e sole le casse
aperte. (Vanno contate le casse aperte). Mi metto in wait finchè */
void *DirettoreCasse(void *arg){
    //Creo un array di id_casse
    pthread_t *id_casse = malloc(sizeof(pthread_t)*cfg.K);
    //Creo un array di id_timer
    pthread_t *id_timer = malloc(sizeof(pthread_t)*cfg.K);
    //Genero cfg.K thread casse
    for(int i=0;i<cfg.K;i++){
        ec_meno1(pthread_create(&id_casse[i],NULL,Casse,&casse[i]),"Errore creazione thread cassa.");
        ec_meno1(pthread_create(&id_timer[i],NULL,TimerCasse,&casse[i]),"Errore creazione thread timer.");
    }
    //Creo un array di id_
    while(sigquit == 0 && sighup == 0){
        break;
        pthread_mutex_lock(&mutexInformazioni);
        while(informazioniRicevute()==0  && sighup==0 && sigquit==0){
            pthread_cond_wait(&condInformazioni,&mutexInformazioni);
        }
        pthread_mutex_unlock(&mutexInformazioni);
    }
    
    //Se sono uscito, attendo il termine dei thread timer e casse.
    for(int i=0;i<cfg.K;i++){
        ec_meno1(pthread_join(id_timer[i],NULL),"Errore join thread timer cassa.");
        print("TIMER CASSA TERMINATO");
        ec_meno1(pthread_join(id_casse[i],NULL),"Errore join thread cassa.");
        print("CASSA TERMINATA");
    }
    //Pulisco
    free(id_casse);
    free(id_timer);
    pthread_exit(NULL);
}
/*Thread ausiliario Direttore Clienti: Si occupa di gestire la coda dei clienti che 
vogliono uscire dal supermercato, e si occupa di far rientrare cfg.E clienti dal momento che
ne sono usciti cfg.E, ossia fa rimanere costante il numero di clienti nel supermercato.
Si occupa anche di avviare i cfg.C thread Clienti e attende quindi il loro termine per
terminare.*/
void *DirettoreClienti(void *arg){
    //Creo un array di clienti
    client *clienti = malloc(sizeof(client)*cfg.C);
    pthread_t *id_clienti = malloc(sizeof(pthread_t)*cfg.C);
    //Avvio i clienti
    for(int i=0;i<cfg.C;i++){
        inizializzaCliente(&clienti[i],i);
        ec_meno1(pthread_create(&id_clienti[i],NULL,Clienti,&clienti[i]),"Errore creazione cliente");
    }
    int *clientiUscenti = malloc(sizeof(int)*cfg.E);
    int k=0;
    int contatore = 0;
    while(sighup == 0 && sigquit == 0){
        pthread_mutex_lock(&mutexCodaDirettore);
        while(isEmpty(*codaDirettore) && sighup==0 && sigquit==0){
            pthread_cond_wait(&condCodaDirettore,&mutexCodaDirettore);
        }
        pthread_mutex_unlock(&mutexCodaDirettore);
        if(sighup || sigquit) break;
        //Se sono qui, devo salvarmi i clienti della coda in un array
        pthread_mutex_lock(&mutexCodaDirettore);
        if(!isEmpty(*codaDirettore)){
            client attuale = codaDirettore->head->c;
            clientiUscenti[k]= attuale.id;
            k++;
            dequeue(codaDirettore);
            pthread_mutex_unlock(&mutexCodaDirettore);
            //Controllo se k=cfg.E:
            if(k==cfg.E){
                client *clients = malloc(sizeof(client)*cfg.E);
                for(int i=0;i<k;i++){
                    unsigned int seed1 = clientiUscenti[i] + time(NULL);
                    long r = generaProdotto(seed1,cfg.P);
                    clients[i].id = clientiUscenti[i];
                    clients[i].numProd = (int)r;
                    //Assegno il tempo per acquistare i prodotti (da 10 a T)
                    unsigned int seed2 = clients[i].id + time(NULL);
                    r = generaTempoAcquisto(seed2,cfg.T);
                    clients[i].tempoAcquisto = (float)r/1000;
                    long nanoSec = clients[i].tempoAcquisto * 1000000000;
                    //creo la struttura per i nanosecondi:
                    struct timespec t = {0, nanoSec};
                    nanosleep(&t,NULL);
                    if(clients[i].numProd==0){
                        pthread_mutex_lock(&mutexCodaDirettore);
                        enqueue(codaDirettore,clients[i]);
                        pthread_cond_signal(&condCodaDirettore);
                        pthread_mutex_unlock(&mutexCodaDirettore);
                    }else{
                        clients[i].numCodeViste++;
                        pthread_mutex_lock(&mutexCasse);
                        int max = contaCasseAperte(casse);
                        //genero un array con sole le casse aperte
                        cassiere *casseOpen;
                        if(max>0)
                            casseOpen = casseAperte(casse, max);
                        pthread_mutex_unlock(&mutexCasse);
                        if(max==0) break;//segnale

                        //Ora mi creo un seed
                        unsigned int seed3 = clients[i].id + time(NULL);
                        long cas = rand_r(&seed3)%(max);
                        pthread_mutex_lock(&mutexCodeCassieri[casseOpen[cas].id]);
                        enqueue(codeCassieri[casseOpen[cas].id],clients[i]);
                        pthread_cond_signal(&condCodeCassieri[casseOpen[cas].id]);
                        pthread_mutex_unlock(&mutexCodeCassieri[casseOpen[cas].id]);

                    }
                }
                k=0;
            }
        }
        pthread_mutex_unlock(&mutexCodaDirettore);
    }
    
    //Joino i clienti
    for(int i=0;i<cfg.C;i++){
        ec_meno1(pthread_join(id_clienti[i],NULL),"Errore join cliente");
        print("CLIENTE TERMINATO.");
    }
    //Pulisco
    free(id_clienti);
    free(clienti);
    free(clientiUscenti);
    pthread_exit(NULL);
}
/*Thread ausiliario per ogni cassa. Serve per controllare lo stato di ogni cassa e mandare
una "notifica" al direttore ogni cfg.S millisecondi per ciascuna cassa, solamente quando queste
sono aperte. Quando sono chiuse, il timer non deve partire.*/
void *TimerCasse(void *arg){
    while(sighup == 0 && sigquit==0){
        pthread_mutex_lock(&mutexCasseChiuse[((cassiere *)arg)->id -1]);
        while(casseChiuse[((cassiere *)arg)->id -1] && sighup == 0 && sigquit == 0){
            pthread_cond_wait(&condCasseChiuse[((cassiere *)arg)->id -1],&mutexCasseChiuse[((cassiere *)arg)->id -1]);
        }
        pthread_mutex_unlock(&mutexCasseChiuse[((cassiere *)arg)->id -1]);
        //Se sono qui, o ho ricevuto uno dei due segnali:
        if(sighup || sigquit) break;
        //Oppure la cassa corrispondente a me ha aperto.
        //Avvio quindi il timer:
        //Devo attendere cfg.tempo_info millisecondi per poi avvisare il direttore
        long r = cfg.tempo_info * 1000000;
        struct timespec t = {0,r};
        nanosleep(&t,NULL);
        //Ho atteso il tempo necessario, ora devo dire al direttore che ho aggiornato
        //i dati sul mio cassiere e mandargli un segnale:
        pthread_mutex_lock(&mutexCodeCassieri[((cassiere *)arg)->id -1]);
        int dim = codeCassieri[((cassiere *)arg)->id -1]->dim;
        pthread_mutex_unlock(&mutexCodeCassieri[((cassiere *)arg)->id -1]);
        pthread_mutex_lock(&mutexInformazioni);
        arrayInformazioni[((cassiere *)arg)->id -1] = dim;
        pthread_cond_signal(&condInformazioni);
        pthread_mutex_unlock(&mutexInformazioni);
    }
    pthread_exit(NULL);
}
/*Thread Casse: Serve per gestire una coda di Clienti. Ciascuna Cassa, quando è chiusa, attende
di essere aperta. Quando invece è aperta, attende di avere clienti nella sua coda. Appena la 
coda dei clienti relativi alla i-esima cassa è non vuota, attende un tempo random tra 
T_MIN_CASS e T_MAX_CASS in millisecondi, dopodichè prende il controllo della coda e inizia a
servire il primo cliente. Dopo averlo servito, lo manda nella coda del direttore e lo toglie dalla
propria coda. Attende di nuovo un tempo random come sopra e serve il prossimo, se c'è.
NOTE:
Se la cassa viene chiusa e se ci sono dei clienti in coda, il cassiere invece di servire i clienti
successivi li fa spostare in una delle altre casse aperte. Le casse sono cfg.K.*/
void *Casse(void *arg){
    float tempoTot = 0;
    int numProdElab =0;
    int numClientiElab = 0;
    float tempoCoda = 0;
    //Devo elaborare i clienti alla mia coda fino a che non ricevo un segnale
    while(sighup == 0 && sigquit==0){
        //RIMANGO IN ATTESA FINCHÈ SON CHIUSA:
        pthread_mutex_lock(&mutexCasseChiuse[((cassiere *)arg)->id -1]);
        while(casseChiuse[((cassiere *)arg)->id -1] && sighup==0 && sigquit==0){
            tempoTot=0;
            numProdElab =0;
            numClientiElab =0;
            tempoCoda = 0;
            pthread_cond_wait(&condCasseChiuse[((cassiere *)arg)->id -1],&mutexCasseChiuse[((cassiere *)arg)->id -1]);
        }
        pthread_mutex_unlock(&mutexCasseChiuse[((cassiere *)arg)->id -1]);
        //SONO STATO APERTO, RIMANGO IN ATTESA FINCHÈ HO LA CODA VUOTA
        //SE SONO USCITO PER UN SEGNALE ROMPO!
        if(sighup || sigquit) break;
        pthread_mutex_lock(&mutexCodeCassieri[((cassiere *)arg)->id -1]);
        while(isEmpty(*codeCassieri[((cassiere *)arg)->id -1]) && sighup==0 && sigquit==0){
            tempoCoda=0;
            pthread_cond_wait(&condCodeCassieri[((cassiere *)arg)->id -1],&mutexCodeCassieri[((cassiere *)arg)->id -1]);
        }
        pthread_mutex_unlock(&mutexCodeCassieri[((cassiere *)arg)->id -1]);
        //SE SONO USCITO PER UN SEGNALE ROMPO!
        if(sighup || sigquit) break;
        //HO FINITO DI ATTENDERE!
        //Attendo il mio tempo fisso
        //converto il tempo fisso in nanosecondi
        long nanoSec = ((cassiere *)arg)->tempoFisso * 1000000000;
        tempoTot = tempoTot + ((cassiere *)arg)->tempoFisso;
        tempoCoda = tempoCoda + ((cassiere *)arg)->tempoFisso;
        struct timespec t = {0, nanoSec};
        nanosleep(&t,NULL);
        //ORA INIZIO A GESTIRE LA CODA:
        pthread_mutex_lock(&mutexCodeCassieri[((cassiere *)arg)->id -1]);
        if(!isEmpty(*codeCassieri[((cassiere *)arg)->id -1])){
            client attuale = codeCassieri[((cassiere *)arg)->id -1]->head->c;
            numClientiElab++;
            pthread_mutex_unlock(&mutexCodeCassieri[((cassiere *)arg)->id -1]);
            //Attendo il tempo del prodotto per numProdotti volte:
            long nanoSec2 = ((cassiere *)arg)->tempoProdotto * 1000000000;
            tempoTot = tempoTot + ((cassiere *)arg)->tempoProdotto *attuale.numProd;
            tempoCoda = tempoCoda + ((cassiere *)arg)->tempoProdotto *attuale.numProd;
            numProdElab = numProdElab + attuale.numProd;
            struct timespec t2 = {0,nanoSec2};
            //Ora attendo ((cassiere *)arg)->tempoProdotto per un ciclo che va da 0 a attuale->numProd
            for(int i=0;i<attuale.numProd;i++){
                nanosleep(&t2,NULL);
            }
            //Il cliente è stato servito, lo metto in coda al direttore
            pthread_mutex_lock(&mutexCodaDirettore);
            enqueue(codaDirettore,attuale);
            pthread_cond_signal(&condCodaDirettore);
            pthread_mutex_unlock(&mutexCodaDirettore);
            //Ora lo posso togliere dalla mia coda:
            pthread_mutex_lock(&mutexCodeCassieri[((cassiere *)arg)->id -1]);
            dequeue(codeCassieri[((cassiere *)arg)->id -1]);
            pthread_mutex_unlock(&mutexCodeCassieri[((cassiere *)arg)->id -1]);
            //Ora ho finito con questo cliente, posso stampare il suo resoconto:
            //LOGFILE
            pthread_mutex_lock(&mutexLogFile);
            fprintf(stdout,"| ID CLIENTE: %d | N. PRODOTTI ACQUISTATI: %d | TEMPO TOT SUPERMERCATO: %.3f s | TEMPO TOT SPESO IN CODA: %.3f s | N. CODE VISTE: %d |\n", attuale.id, attuale.numProd, attuale.tempoAcquisto + tempoCoda, tempoCoda, attuale.numCodeViste);
            fflush(stdout);
            pthread_mutex_unlock(&mutexLogFile);
            //Ora controllo se mentre servivo il cliente non mi abbiano chiuso:
            int hoChiuso=0;
            pthread_mutex_lock(&mutexCasseChiuse[((cassiere *)arg)->id -1]);
            if(casseChiuse[((cassiere *)arg)->id -1]){
                hoChiuso=1;
            }
            pthread_mutex_unlock(&mutexCasseChiuse[((cassiere *)arg)->id -1]);
            //Se ho chiuso, devo sloggiare dalla mia coda tutti i restanti:
            if(hoChiuso){
                //Locko e scorro tutta la coda:
                pthread_mutex_lock(&mutexCodeCassieri[((cassiere *)arg)->id -1]);
                while(!isEmpty(*codeCassieri[((cassiere *)arg)->id -1])){
                    client aux = codeCassieri[((cassiere *)arg)->id -1]->head->c;
                    dequeue(codeCassieri[((cassiere *)arg)->id -1]);
                    //Assegno al cliente estratto una nuova cassa, e gli aumento il numero di code viste:
                    aux.numCodeViste++;
                    pthread_mutex_lock(&mutexCasse);
                    int max = contaCasseAperte(casse);
                    cassiere *casseOpen;
                    if(max>0){}
                        casseOpen = casseAperte(casse,max);
                    pthread_mutex_unlock(&mutexCasse);
                    //Se è 0 ho chiuso per il segnale
                    if(max==0) break;
                    unsigned int seed3 = aux.id + time(NULL);
                    long cas = generaCassiere(&seed3,max);
                    //Infilo nella nuova coda, segnalo, svuoto casseOpen
                    pthread_mutex_lock(&mutexCodeCassieri[casseOpen[cas].id]);
                    enqueue(codeCassieri[casseOpen[cas].id],aux);
                    pthread_cond_signal(&condCodeCassieri[casseOpen[cas].id]);
                    pthread_mutex_unlock(&mutexCodeCassieri[casseOpen[cas].id]);
                    free(casseOpen);
                }
                pthread_mutex_unlock(&mutexCodeCassieri[((cassiere *)arg)->id -1]);
                //Ora scrivo lo stato della cassa, nel logFile:
                pthread_mutex_lock(&mutexLogFile);
                fprintf(stdout,"| ID CASSA: %d | N. PROD. ELAB.: %d | N. Clienti: %d | TEMPO TOT: %.3f s | TEMPO MEDIO: %.3f s | N. CHIUSURE: %d |\n",((cassiere *)arg)->id, numProdElab, numClientiElab, tempoTot, tempoTot/numClientiElab, ((cassiere *)arg)->numeroChiusure);
                fflush(stdout);
                pthread_mutex_unlock(&mutexLogFile);
            }
        }
        pthread_mutex_unlock(&mutexCodeCassieri[((cassiere *)arg)->id -1]);
        

    }
    //Gestisco i segnali:
    //Se arrivo qui la cassa chiude una volta:
    ((cassiere *)arg)->numeroChiusure++;
    if(sighup){
        int chiusa=0;
        //Se la cassa è chiusa, esco semplicemente, quindi:
        pthread_mutex_lock(&mutexCasseChiuse[((cassiere *)arg)->id -1]);
        chiusa = casseChiuse[((cassiere *)arg)->id -1];
        pthread_mutex_unlock(&mutexCasseChiuse[((cassiere *)arg)->id -1]);
        if(!chiusa){
            //Attendo il tempo fisso
            long nanoSec = ((cassiere *)arg)->tempoFisso * 1000000000;
            tempoTot = tempoTot + ((cassiere *)arg)->tempoFisso;
            tempoCoda = tempoCoda + ((cassiere *)arg)->tempoFisso;
            struct timespec t = {0, nanoSec};
            nanosleep(&t,NULL);
            //Mi creo una variabile per capire se stampare o meno i dati della cassa:
            int stampa=0;
            //Locko la coda, servo tutti finchè ci sono, infine chiudo, stampo ed esco
            pthread_mutex_lock(&mutexCodeCassieri[((cassiere *)arg)->id -1]);
            while(!isEmpty(*codeCassieri[((cassiere *)arg)->id -1])){
                stampa = 1;
                client attuale = codeCassieri[((cassiere *)arg)->id -1]->head->c;
                numClientiElab++;
                //Attendo il tempo del prodotto per numProdotti volte:
                long nanoSec2 = ((cassiere *)arg)->tempoProdotto * 1000000000;
                tempoTot = tempoTot + ((cassiere *)arg)->tempoProdotto *attuale.numProd;
                tempoCoda = tempoCoda + ((cassiere *)arg)->tempoProdotto *attuale.numProd;
                numProdElab = numProdElab + attuale.numProd;
                struct timespec t2 = {0,nanoSec2};
                //Ora attendo ((cassiere *)arg)->tempoProdotto per un ciclo che va da 0 a attuale->numProd
                for(int i=0;i<attuale.numProd;i++){
                    nanosleep(&t2,NULL);
                }
                //Il cliente è stato servito, lo metto in coda al direttore
                pthread_mutex_lock(&mutexCodaDirettore);
                enqueue(codaDirettore,attuale);
                pthread_cond_signal(&condCodaDirettore);
                pthread_mutex_unlock(&mutexCodaDirettore);
                //Ora lo posso togliere dalla mia coda:
                dequeue(codeCassieri[((cassiere *)arg)->id -1]);
                //Ora ho finito con questo cliente, posso stampare il suo resoconto:
                //LOGFILE
                pthread_mutex_lock(&mutexLogFile);
                fprintf(stdout,"| ID CLIENTE: %d | N. PRODOTTI ACQUISTATI: %d | TEMPO TOT SUPERMERCATO: %.3f s | TEMPO TOT SPESO IN CODA: %.3f s | N. CODE VISTE: %d |\n", attuale.id, attuale.numProd, attuale.tempoAcquisto + tempoCoda, tempoCoda, attuale.numCodeViste);
                fflush(stdout);
                pthread_mutex_unlock(&mutexLogFile);
            }
            pthread_mutex_unlock(&mutexCodeCassieri[((cassiere *)arg)->id -1]);
            if(stampa){
                //Ora scrivo lo stato della cassa, nel logFile:
                pthread_mutex_lock(&mutexLogFile);
                fprintf(stdout,"| ID CASSA: %d | N. PROD. ELAB.: %d | N. Clienti: %d | TEMPO TOT: %.3f s | TEMPO MEDIO: %.3f s | N. CHIUSURE: %d |\n",((cassiere *)arg)->id, numProdElab, numClientiElab, tempoTot, tempoTot/numClientiElab, ((cassiere *)arg)->numeroChiusure);
                fflush(stdout);
                pthread_mutex_unlock(&mutexLogFile);
            }
        }
    }else if(sigquit){
        //Qui invece devo servire un cliente e poi uscire:
        int chiusa=0;
        //Se la cassa è chiusa, esco semplicemente, quindi:
        pthread_mutex_lock(&mutexCasseChiuse[((cassiere *)arg)->id -1]);
        chiusa = casseChiuse[((cassiere *)arg)->id -1];
        pthread_mutex_unlock(&mutexCasseChiuse[((cassiere *)arg)->id -1]);
        if(!chiusa){
            //Attendo il tempo fisso
            long nanoSec = ((cassiere *)arg)->tempoFisso * 1000000000;
            tempoTot = tempoTot + ((cassiere *)arg)->tempoFisso;
            tempoCoda = tempoCoda + ((cassiere *)arg)->tempoFisso;
            struct timespec t = {0, nanoSec};
            nanosleep(&t,NULL);
            int stampa=0;
            pthread_mutex_lock(&mutexCodeCassieri[((cassiere *)arg)->id -1]);
            if(!isEmpty(*codeCassieri[((cassiere *)arg)->id -1])){
                stampa=1;
                client attuale = codeCassieri[((cassiere *)arg)->id -1]->head->c;
                numClientiElab++;
                //Attendo il tempo del prodotto per numProdotti volte:
                long nanoSec2 = ((cassiere *)arg)->tempoProdotto * 1000000000;
                tempoTot = tempoTot + ((cassiere *)arg)->tempoProdotto *attuale.numProd;
                tempoCoda = tempoCoda + ((cassiere *)arg)->tempoProdotto *attuale.numProd;
                numProdElab = numProdElab + attuale.numProd;
                struct timespec t2 = {0,nanoSec2};
                //Ora attendo ((cassiere *)arg)->tempoProdotto per un ciclo che va da 0 a attuale->numProd
                for(int i=0;i<attuale.numProd;i++){
                    nanosleep(&t2,NULL);
                }
                //Il cliente è stato servito, lo metto in coda al direttore
                pthread_mutex_lock(&mutexCodaDirettore);
                enqueue(codaDirettore,attuale);
                pthread_cond_signal(&condCodaDirettore);
                pthread_mutex_unlock(&mutexCodaDirettore);
                //Ora lo posso togliere dalla mia coda
                dequeue(codeCassieri[((cassiere *)arg)->id -1]);
                //Ora ho finito con questo cliente, posso stampare il suo resoconto:
                //LOGFILE
                pthread_mutex_lock(&mutexLogFile);
                fprintf(stdout,"| ID CLIENTE: %d | N. PRODOTTI ACQUISTATI: %d | TEMPO TOT SUPERMERCATO: %.3f s | TEMPO TOT SPESO IN CODA: %.3f s | N. CODE VISTE: %d |\n", attuale.id, attuale.numProd, attuale.tempoAcquisto + tempoCoda, tempoCoda, attuale.numCodeViste);
                fflush(stdout);
                pthread_mutex_unlock(&mutexLogFile);
            }
            pthread_mutex_unlock(&mutexCodeCassieri[((cassiere *)arg)->id -1]);
            if(stampa){
                //Ora scrivo lo stato della cassa, nel logFile:
                pthread_mutex_lock(&mutexLogFile);
                fprintf(stdout,"| ID CASSA: %d | N. PROD. ELAB.: %d | N. Clienti: %d | TEMPO TOT: %.3f s | TEMPO MEDIO: %.3f s | N. CHIUSURE: %d |\n",((cassiere *)arg)->id, numProdElab, numClientiElab, tempoTot, tempoTot/numClientiElab, ((cassiere *)arg)->numeroChiusure);
                fflush(stdout);
                pthread_mutex_unlock(&mutexLogFile);
            }
        }
    }
    pthread_exit(NULL);
}
/*Thread Cliente: Questo thread serve per memorizzare dei dati relativi a ciascun cliente. Ogni
cliente ha un id, sono in totale cfg.C. Appena entrati nel supermercato (all'inizio entrano tutti
concorrentemente) generano un numero casuale corrispondente al numero di prodotti che acquisteranno,
(massimo: cfg.P), un numero casuale (minimo T_MIN_ACQUISTI, massimo cfg.T) corrispondente al tempo
che impiegheranno ad acquistare. Dopo aver acquistato, ciascun cliente sceglie una coda random 
tra quelle delle casse aperte disponibili e ci si mette in coda. Il cliente di per sè non fa altro.*/
void *Clienti(void *arg){
    //Generazione casuale numero prodotti (da 0 a cfg.P):
    unsigned int seed1 = ((client *)arg)->id + time(NULL);
    long r = generaProdotto(seed1, cfg.P);
    ((client *)arg)->numProd = (int)r;

    //Generazione casuale tempo per acquisto (da 10 a cfg.T)
    unsigned int seed2 = ((client *)arg)->id + time(NULL);
    r = generaTempoAcquisto(seed2, cfg.T);
    ((client *)arg)->tempoAcquisto = (float)r/1000;
    //Converto quel tempo in nanosecondi, per poter usare la 
    //nanosleep. 
    long nanoSec = ((client *)arg)->tempoAcquisto * 1000000000;
    struct timespec t = {0, nanoSec};
    nanosleep(&t,NULL);
    //Ora che ho aspettato il tempo di acquisto, devo vedere 
    //quanti prodotti ho acquistato: Se 0, vado dal direttore,
    //altrimenti scelgo una cassa a caso. Dopodichè ho finito.
    if(((client *)arg)->numProd == 0){
        //Vado dal direttore!
        //Locko la coda
        pthread_mutex_lock(&mutexCodaDirettore);
        //Inserisco in coda
        enqueue(codaDirettore,*((client *)arg));
        //Segnalo il direttore
        pthread_cond_signal(&condCodaDirettore);
        //Unlocko la coda
        pthread_mutex_unlock(&mutexCodaDirettore);
    }else{
        ((client *)arg)->numCodeViste++;
        //Scelgo una delle casse aperte!
        //Intanto le conto le casse aperte:
        pthread_mutex_lock(&mutexCasse);
        int casseAp = contaCasseAperte(casse);
        pthread_mutex_unlock(&mutexCasse);
        //Ipotizzo casse aperte >0 sempre, quando chiude gestisco:
        if(casseAp == 0) pthread_exit(NULL); //faccio robe
        //Creo un array di casse aperte tra cui scegliere
        pthread_mutex_lock(&mutexCasse);
        cassiere *openCass = casseAperte(casse,casseAp);
        pthread_mutex_unlock(&mutexCasse);
        //Ora ne scelgo una random
        unsigned int seed3 = ((client *)arg)->id + time(NULL);
        long cassaScelta = generaCassiere(&seed3,casseAp);
        //Metto in coda e segnalo il cassiere
        pthread_mutex_lock(&mutexCodeCassieri[openCass[cassaScelta].id]);
        enqueue(codeCassieri[openCass[cassaScelta].id],*((client *)arg));
        pthread_cond_signal(&condCodeCassieri[openCass[cassaScelta].id]);
        pthread_mutex_unlock(&mutexCodeCassieri[openCass[cassaScelta].id]);
        //Libero la memoria
        free(openCass);
    }
    pthread_exit(NULL);
}
//FUNZIONI DI INIZIALIZZAZIONE CONFIG
void inizializzaConfig(){
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    //Apro il file
    fp = fopen("config.txt", "r");
    ec_null(fp,"Errore nell'apertura del config");

    //Controllo quale linea sto leggendo
    //Converto ogni linea a intero
    int i=0;
    while ((read = getline(&line, &len, fp)) != -1) {
        switch(i){
            case 0:
                cfg.K = atoi(&line[2]);
                i++;
            break;
            case 1:
                cfg.C = atoi(&line[2]);
                i++;
            break;
            case 2:
                cfg.E = atoi(&line[2]);
                i++;
            break;
            case 3:
                cfg.T = atoi(&line[2]);
                i++;
            break;
            case 4:
                cfg.P = atoi(&line[2]);
                i++;
            break;
            case 5:
                cfg.S = atoi(&line[2]);
                i++;
            break;
            case 6:
                cfg.S1 = atoi(&line[3]);
                i++;
            break;
            case 7:
                cfg.S2 = atoi(&line[3]);
                i++;
            break;
        }
    }
    //Chiudo il file
    fclose(fp);
    if (line)
        free(line);

    //TODO: aggiungere anche questi nel config.txt
    cfg.tempo_info = 20;
    cfg.casse_iniziali = 3;
    cfg.num_clienti = 0;
    cfg.num_prodotti = 0;
}

//FUNZIONE DI INIZIALIZZAZIONE GENERALE
void initAll(){
    //Coda direttori
    codaDirettore = creaCoda();
    //Cassieri
    casse = malloc(sizeof(cassiere)*cfg.K);
    codeCassieri = malloc(sizeof(coda *)*cfg.K);
    mutexCodeCassieri = malloc(sizeof(pthread_mutex_t)*cfg.K);
    condCodeCassieri = malloc(sizeof(pthread_cond_t)*cfg.K);
    arrayInformazioni = malloc(sizeof(int)*cfg.K);
    casseChiuse = malloc(sizeof(int)*cfg.K);
    mutexCasseChiuse = malloc(sizeof(pthread_mutex_t)*cfg.K);
    condCasseChiuse = malloc(sizeof(pthread_cond_t)*cfg.K);
    for(int i=0;i<cfg.K;i++){
        inizializzaCassiere(&casse[i],i,cfg.S);
        //Apro inoltre cfg.casse_iniziali casse
        arrayInformazioni[i]=-2;
        casseChiuse[i] = 1;
        if(i<cfg.casse_iniziali){
            apriCassa(&casse[i]);
            arrayInformazioni[i]=-1;
            casseChiuse[i] =0;
        }
        codeCassieri[i] = creaCoda();
        pthread_mutex_init(&mutexCodeCassieri[i],NULL);
        pthread_cond_init(&condCodeCassieri[i],NULL);
        pthread_cond_init(&condCasseChiuse[i],NULL);
        pthread_mutex_init(&mutexCasseChiuse[i],NULL);
    }
}

//FUNZIONE DI PULIZIA GENERALE
void cleanAll(){
    free(codaDirettore);
    free(mutexCodeCassieri);
    free(condCodeCassieri);
    for(int i=0;i<cfg.K;i++){
        free(codeCassieri[i]);
    }
    free(casse);
    free(codeCassieri);
}
//FUNZIONE PER GESTIRE I SEGNALI - HANDLER
void handler(int signum){
     if(signum == 3){
        //SIGQUIT (CTRL+\)
        fprintf(stdout,"RICEVUTO SEGNALE SIGQUIT\n");
        sigquit = 1;
        //Segnalo tutti i timer
        for(int i=0;i<cfg.K;i++){
            pthread_cond_broadcast(&condCasseChiuse[i]);
            pthread_cond_signal(&condCodeCassieri[i]);
        }
        //exit(EXIT_SUCCESS);
    }else if(signum == 1){
        //SIGHUP (KILL(1))
        fprintf(stdout,"RICEVUTO SEGNALE SIGHUP\n");
        sighup = 1;
        //Segnalo tutti i timer
        for(int i=0;i<cfg.K;i++){
            pthread_cond_broadcast(&condCasseChiuse[i]);
            pthread_cond_signal(&condCodeCassieri[i]);
        }
        //exit(EXIT_SUCCESS);
    }
}












//FUNZIONI AGGIUNTIVE
int contaCasseAperte(cassiere *c){
    int count =0;
    for(int i=0;i<cfg.K;i++){
        if(c[i].cassaAperta == 1){
            count++;
        }
    }
    return count;
}

cassiere *casseAperte(cassiere *c, int count){
    cassiere *cOpen = malloc(sizeof(cassiere)*count);
    int k=0;
    int tmp = 0;
    for(int i=0;i<cfg.K;i++){
        if(c[i].cassaAperta == 1){
            tmp = c[i].id;
            tmp--;
            cOpen[k].id = tmp;
            k++;
        }
    }
    return cOpen;
}

void print(char *s){
    fprintf(stdout,"%s\n",s);
    fflush(stdout);
}

int *queueDimensions(){
    int *dims = malloc(sizeof(int)*cfg.K);
    for(int i=0;i<cfg.K;i++){
        pthread_mutex_lock(&mutexCodeCassieri[i]);
        dims[i] = codeCassieri[i]->dim;
        pthread_mutex_unlock(&mutexCodeCassieri[i]);
    }
    return dims;
}
//controllo tutte le info
int informazioniRicevute(){
    int vero=0;
    int count=0;
    for(int i=0;i<cfg.K;i++){
        if(arrayInformazioni[i] != -1){
            count++;
        }
    }
    if(count == cfg.K){
        vero=1;
    }
    return vero;
}

void ritornaMenoUno(){
    for(int i=0;i<cfg.K;i++){
        if(arrayInformazioni[i] != -2){
            arrayInformazioni[i] = -1;
        }
    }
}
int esseUno(){
    int vero=0;
    int count=0;
    pthread_mutex_lock(&mutexInformazioni);
    for(int i=0;i<cfg.K;i++){
        if(arrayInformazioni[i] != -1 && arrayInformazioni[i] != -2 && arrayInformazioni[i]<=1){
            count++;
        }
    }
    pthread_mutex_unlock(&mutexInformazioni);
    if(count >= cfg.S1){
        vero =1;
    }
    return vero;
}
int esseDue(){
    int vero=0;
    pthread_mutex_lock(&mutexInformazioni);
    for(int i=0;i<cfg.K;i++){
        if(arrayInformazioni[i] != -1 && arrayInformazioni[i] != -2 && arrayInformazioni[i]>= cfg.S2){
            vero=1;
        }
    }
    pthread_mutex_unlock(&mutexInformazioni);
    return vero;
}