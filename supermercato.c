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
/*STRUTTURA DEL CONFIG, CHE CONTIENE TUTTI I DATI*/
typedef struct config{
    int K; //casse max apribili
    int C; //clienti totali
    int E; //chi esce/entra
    int T; //tempo max per gli acquisti client
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

typedef struct supermercato{
    int numeroProd; //numero prodotti acquistati
    int numClientiTot; //numero clienti totali
}supermercato;

/*PROTOTIPI DI FUNZIONI*/
//funzione che inizializza il config
void inizializzaConfig();
//Variabile globale per il file di configurazione
config cfg;
//Variabili globali per gestire i segnali SIGHUP e SIGQUIT
volatile sig_atomic_t sighup = 0;
volatile sig_atomic_t sigquit = 0;

FILE *flog;
void handler(int signum);


//file aggiuntivi
#include "./headers/cassiere.h"
#include "./headers/client.h"
#include "./errors/error.h"
#include "./headers/coda.h"

//Creo le due code globali, con le rispettive mutex (e anche dei flag per dire se le code sono vuote o no)
//CODA PER DIRETTORE
coda *codaDirettore;
pthread_mutex_t mutexCodaDirettore = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condCodaDirettore = PTHREAD_COND_INITIALIZER;
int codaDirettoreVuota = 1;
//Mi serve una mutex/cond anche per il flag, poichè sarà poi il cliente a settarla a 0
pthread_mutex_t mutexFlagCodaVuotaDirettore = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condFlagCodaVuotaDirettore = PTHREAD_COND_INITIALIZER;


//I CASSIERI SONO K, QUINDI CREO ARRAY DI MUTEX E DI CV, NONCHÉ DI FLAG E CODE
coda **codeCassieri;
pthread_mutex_t *mutexCodeCassieri;
//Mutex per accedere al logfile
pthread_mutex_t mutexLogFile = PTHREAD_MUTEX_INITIALIZER;
//COND di codeCassieri che mi serve quando il direttore si mette in attesa prima di prendersi la coda che ha il cassiere 
pthread_cond_t *condCodeCassieri;
int *codeCassieriVuote;
//Mi creo un array di interi flag che segnalano al direttore che il cassiere ha finito di servire il cliente in testa
int *flagCodeCassieri;
//Mi serve una mutex/cond anche per il flag, poichè sarà poi il cliente a settarla a 0
pthread_mutex_t *mutexFlagCodeVuoteCassieri;
pthread_cond_t *condFlagCodeVuoteCassieri;
//cassieri
pthread_mutex_t mutexCassieri = PTHREAD_MUTEX_INITIALIZER;
cassiere *cassieri;
pthread_mutex_t *mutexCasseChiuse;
pthread_cond_t *condCasseChiuse;
int *casseChiuse;
//Creo le informazioni di ciascun cassiere per il thread TimerCassa
pthread_mutex_t *mutexInfoCasse;
int *infoCasse;
pthread_mutex_t mutexFlagInfoCasse = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condFlagInfoCasse = PTHREAD_COND_INITIALIZER;
int flagInfoCasse;
//funzione che inizializza le mutex/cv/flags e code dei cassieri
void initCassieri();

//Funzione per il thread Cliente
void *Cliente(void *arg);
//Funzione per il thread Cassiere
void *Cassiere(void *arg);
//Funzione per il thread Direttore per gestire i clienti
void *DirettoreClienti(void *arg);
//Funzione per il thread Direttore per gestire le casse
void *DirettoreCasse(void *arg);
//Funzione per il thread Timer, che per ogni cassiere invia ogni cfg.tempo_info millisecondi al direttore
//le condizioni relative alla coda di quella cassa (la lunghezza).
void *TimerCassa(void *arg);
int main(){
    //INIZIALIZZO GESTORE SEGNALI
    struct sigaction s;
    memset(&s,0,sizeof(s));
    s.sa_handler=handler;
    ec_meno1(sigaction(SIGHUP,&s,NULL),"Errore segnale di SIGHUP");
    ec_meno1(sigaction(SIGQUIT,&s,NULL),"Errore segnale di SIGQUIT");
    //inizializzo il config (che poi leggerà dal file config.txt)
    inizializzaConfig();
    //inizializzo la coda del direttore dei clienti che non acquistano
    codaDirettore = creaCoda();
    //GENERO I CASSIERI
    pthread_t *id_casse = malloc(sizeof(pthread_t)*cfg.K);
    cassieri = malloc(sizeof(cassiere)*cfg.K);
    initCassieri();
    for(int i=0;i<cfg.K;i++){
        id_casse[i] = i;
        inizializzaCassiere(&cassieri[i],id_casse[i]);
        if(i<cfg.casse_iniziali){
            apriCassa(&cassieri[i]);
        }
        if(pthread_create(&id_casse[i],NULL,Cassiere,&cassieri[i]) != 0){
            fprintf(stderr,"Errore creazione thread cassiere %d-esimo",i);
            exit(EXIT_FAILURE);
        }
        if(pthread_create(&id_casse[i],NULL,TimerCassa,&cassieri[i]) != 0){
            fprintf(stderr,"Errore creazione thread TimerCassa %d-esimo",i);
            exit(EXIT_FAILURE);
        }
    }
    //Genero cfg->C clienti
    client *cl = malloc(sizeof(client)*cfg.C);
    pthread_t *thids = malloc(sizeof(pthread_t)*cfg.C);
    for(int i=0;i<cfg.C;i++){
        thids[i] = i;
        inizializzaCliente(&cl[i],i);
        if(pthread_create(&thids[i],NULL,Cliente,&cl[i]) != 0){
            fprintf(stderr,"Errore creazione thread %d-esimo",i);
            exit(EXIT_FAILURE);
        }
    }
    //GENERO IL DIRETTORE
    pthread_t id_dir = 1;
    if(pthread_create(&id_dir,NULL,DirettoreClienti,NULL) != 0){
        fprintf(stderr,"Errore creazione thread direttore");
        exit(EXIT_FAILURE);
    }
    //Genero il sottothread DirettoreCasse
    pthread_t id_dir2 = 1;
    int numeroACaso = 7;
    if(pthread_create(&id_dir2,NULL,DirettoreCasse,&numeroACaso) != 0){
        fprintf(stderr,"Errore creazione thread Direttore Cassa");
        exit(EXIT_FAILURE);
    }
    //QUI AVVERRÀ IL PROGRAMMA
    while(sighup == 0 && sigquit == 0){
        break;
    }

    //JOIN CASSIERI
    for(int i=0;i<cfg.K;i++){
        if(pthread_join(id_casse[i], NULL) != 0){
            fprintf(stderr,"Errore join thread cassiere %d-esimo",i);
            exit(EXIT_FAILURE);
        }
        if(pthread_join(id_casse[i], NULL) != 0){
            fprintf(stderr,"Errore join thread TimerCassa %d-esimo",i);
            exit(EXIT_FAILURE);
        }
    }
    //JOIN CLIENTI
    for(int i=0;i<cfg.C;i++){
        int status;
        if(pthread_join(thids[i],(void *)&status) != 0){
            fprintf(stderr,"Errore join thread %d-esimo",i);
            exit(EXIT_FAILURE);
        }
    }
    //JOIN DIRETTORE
    if(pthread_join(id_dir,NULL) != 0){
        fprintf(stderr,"Errore join thread direttore");
        exit(EXIT_FAILURE);
    }
    free(thids);
    free(id_casse);
    free(cassieri);
    free(cl);
    free(codaDirettore);
    free(codeCassieri);
    free(mutexCodeCassieri);
    free(codeCassieriVuote);
    free(mutexFlagCodeVuoteCassieri);
    free(condFlagCodeVuoteCassieri);
    free(casseChiuse);
    free(mutexCasseChiuse);
    free(condCasseChiuse);
    free(mutexInfoCasse);
    free(infoCasse);
    free(condCodeCassieri);
    free(flagCodeCassieri);
    //FREE SINGOLE CODE
    return 0;
}

//Funzione per il thread Cliente
void *Cliente(void *arg){ 
    //Assegno il numero di prodotti da acquistare in modo casuale (da 0 a P)
    unsigned int seed1 = ((client *)arg)->id + time(NULL);
    long r = generaProdotto(seed1);
    ((client *)arg)->numProd = (int)r;
    //Assegno il tempo per acquistare i prodotti (da 10 a T)
    unsigned int seed2 = ((client *)arg)->id + time(NULL);
    r = generaTempoAcquisto(seed2);
    ((client *)arg)->tempoAcquisto = (float)r/1000;
    //Il cliente ora deve attendere ((client *)arg)->tempoAcquisto secondi per poi mettersi in coda.
    //Per farlo però devo distinguere due casi: Se ((client *)arg)->numProd è 0 va nella coda direttore
    //altrimenti va nella coda globale cassiere.
    //In entrambi i casi devo convertire da secondi a nanosecondi e attendere con nanosleep:
    long nanoSec = ((client *)arg)->tempoAcquisto * 1000000000;
    //creo la struttura per i nanosecondi:
    struct timespec t = {0, nanoSec};
    nanosleep(&t,NULL);
    if(((client *)arg)->numProd == 0){
        //direttore
        //penso alla coda
        pthread_mutex_lock(&mutexCodaDirettore);
        enqueue(codaDirettore,*((client *)arg));
        pthread_mutex_unlock(&mutexCodaDirettore);
        //penso al flag
        pthread_mutex_lock(&mutexFlagCodaVuotaDirettore);
        codaDirettoreVuota = 0;
        pthread_cond_signal(&condFlagCodaVuotaDirettore);
        pthread_mutex_unlock(&mutexFlagCodaVuotaDirettore);
    }else{
        //cassiere
        //Va scelto il cassiere:
        //Conto le casse aperte:
        //Per accedere a cassieri che ora è globale devo farlo in mutua esclusione:
        ((client *)arg)->numCodeViste++;
        pthread_mutex_lock(&mutexCassieri);
        int max = contaCasseAperte(cassieri);
        //genero un array con sole le casse aperte
        cassiere *casseOpen = casseAperte(cassieri, max);
        pthread_mutex_unlock(&mutexCassieri);
        //Ora mi creo un seed
        unsigned int seed3 = ((client *)arg)->id + time(NULL);
        long cas = generaCassiere(&seed3,max);
        //Penso alla coda scelta      
        pthread_mutex_lock(&mutexCodeCassieri[casseOpen[cas].id]);
        enqueue(codeCassieri[casseOpen[cas].id],*((client *)arg));
        pthread_mutex_unlock(&mutexCodeCassieri[casseOpen[cas].id]);
        //penso al flag
        pthread_mutex_lock(&mutexFlagCodeVuoteCassieri[casseOpen[cas].id]);
        codeCassieriVuote[casseOpen[cas].id] = 0;
        pthread_cond_signal(&condFlagCodeVuoteCassieri[casseOpen[cas].id]);
        pthread_mutex_unlock(&mutexFlagCodeVuoteCassieri[casseOpen[cas].id]);
        free(casseOpen);
    }
    return NULL;
}

void *Cassiere(void *arg){
    float tempoTot = 0;
    int numProdElab = 0;
    int numClientiElab = 0;
    float tempoCoda = 0;
    while(1){
        //Se la cassa è chiusa, non faccio nulla:
        pthread_mutex_lock(&mutexCasseChiuse[((cassiere *)arg)->id - 1]);
        while(casseChiuse[((cassiere *)arg)->id - 1]){
            tempoTot=0;
            numProdElab =0;
            numClientiElab =0;
            tempoCoda = 0;
            pthread_cond_wait(&condCasseChiuse[((cassiere *)arg)->id - 1],&mutexCasseChiuse[((cassiere *)arg)->id - 1]);
        }
        pthread_mutex_unlock(&mutexCasseChiuse[((cassiere *)arg)->id - 1]);
        pthread_mutex_lock(&mutexFlagCodeVuoteCassieri[((cassiere *)arg)->id - 1]);
        //finchè la coda è vuota non faccio nulla
        while(codeCassieriVuote[((cassiere *)arg)->id - 1]){
            tempoCoda = 0;
            pthread_cond_wait(&condFlagCodeVuoteCassieri[((cassiere *)arg)->id - 1],&mutexFlagCodeVuoteCassieri[((cassiere *)arg)->id - 1]);
        }
        pthread_mutex_unlock(&mutexFlagCodeVuoteCassieri[((cassiere *)arg)->id - 1]);
        //Se la coda non è vuota, allora attendo il tempo costante random del cassiere
        //converto il tempo fisso in nanosecondi
        long nanoSec = ((cassiere *)arg)->tempoFisso * 1000000000;
        tempoTot = tempoTot + ((cassiere *)arg)->tempoFisso;
        tempoCoda = tempoCoda + ((cassiere *)arg)->tempoFisso;
        struct timespec t = {0, nanoSec};
        nanosleep(&t,NULL);
        //ora posso lockare la coda, e iniziare a servire cliente dopo cliente
        pthread_mutex_lock(&mutexCodeCassieri[((cassiere *)arg)->id - 1]);
        if(!isEmpty(*codeCassieri[((cassiere *)arg)->id - 1])){
            //prendo il cliente attuale
            client attuale = (codeCassieri[((cassiere *)arg)->id - 1]->head)->c;
            numClientiElab++;
            pthread_mutex_unlock(&mutexCodeCassieri[((cassiere *)arg)->id - 1]);
            //converto in nanosecondi il tempo prodotto
            long nanoSec2 = ((cassiere *)arg)->tempoProdotto * 1000000000;
            tempoTot = tempoTot + ((cassiere *)arg)->tempoProdotto *attuale.numProd;
            tempoCoda = tempoCoda + ((cassiere *)arg)->tempoProdotto *attuale.numProd;
            numProdElab = numProdElab + attuale.numProd;
            struct timespec t2 = {0,nanoSec2};
            //Ora attendo ((cassiere *)arg)->tempoProdotto per un ciclo che va da 0 a attuale->numProd
            for(int i=0;i<attuale.numProd;i++){
                nanosleep(&t2,NULL);
            }
            //Inserisco i clienti che ho servito via via nella coda del direttore:
            pthread_mutex_lock(&mutexCodaDirettore);
            enqueue(codaDirettore,attuale);
            pthread_mutex_unlock(&mutexCodaDirettore);

            pthread_mutex_lock(&mutexFlagCodaVuotaDirettore);
            codaDirettoreVuota = 0;
            pthread_mutex_unlock(&mutexFlagCodaVuotaDirettore);
            pthread_cond_signal(&condFlagCodaVuotaDirettore);
            pthread_mutex_lock(&mutexCodeCassieri[((cassiere *)arg)->id - 1]);
            dequeue(codeCassieri[((cassiere *)arg)->id - 1]);
            //LOGFILE
            pthread_mutex_lock(&mutexLogFile);
            fprintf(stdout,"| ID CLIENTE: %d | N. PRODOTTI ACQUISTATI: %d | TEMPO TOT SUPERMERCATO: %.3f s | TEMPO TOT SPESO IN CODA: %.3f s | N. CODE VISTE: %d |\n", attuale.id, attuale.numProd, attuale.tempoAcquisto + tempoCoda, tempoCoda, attuale.numCodeViste);
            fflush(stdout);
            pthread_mutex_unlock(&mutexLogFile);
            //HO FINITO DI SERVIRE, CONTROLLO SE INTANTO SON STATO CHIUSO
            int hoChiuso=0;
            pthread_mutex_lock(&mutexCasseChiuse[((cassiere *)arg)->id - 1]);
            if(casseChiuse[((cassiere *)arg)->id - 1]){
                hoChiuso=1;
            }
            pthread_mutex_unlock(&mutexCasseChiuse[((cassiere *)arg)->id - 1]);
            //Quindi se ho chiuso, devo gestire i clienti in coda da me
            if(hoChiuso == 1){
                //finchè ho clienti
                while(!isEmpty(*codeCassieri[((cassiere *)arg)->id - 1])){
                    client aux = codeCassieri[((cassiere *)arg)->id - 1]->head->c;
                    dequeue(codeCassieri[((cassiere *)arg)->id - 1]);
                    //a ciascun cliente devo assegnare un nuovo numero tra le casse aperte:
                    //STEP PER FARE CIÒ:
                    pthread_mutex_lock(&mutexCassieri);
                    int max = contaCasseAperte(cassieri);
                    //genero un array con sole le casse aperte
                    cassiere *casseOpen = casseAperte(cassieri, max);
                    pthread_mutex_unlock(&mutexCassieri);
                    //Ora mi creo un seed
                    
                    unsigned int seed3 = aux.id + time(NULL);
                    long cas = generaCassiere(&seed3,max);
                    aux.numCodeViste++;
                    //Penso alla coda scelta      
                    pthread_mutex_lock(&mutexCodeCassieri[casseOpen[cas].id]);
                    enqueue(codeCassieri[casseOpen[cas].id],aux);
                    pthread_mutex_unlock(&mutexCodeCassieri[casseOpen[cas].id]);
                    //penso al flag
                    pthread_mutex_lock(&mutexFlagCodeVuoteCassieri[casseOpen[cas].id]);
                    codeCassieriVuote[casseOpen[cas].id] = 0;
                    pthread_cond_signal(&condFlagCodeVuoteCassieri[casseOpen[cas].id]);
                    pthread_mutex_unlock(&mutexFlagCodeVuoteCassieri[casseOpen[cas].id]);
                    free(casseOpen);
                }
                //LOGFILE
                pthread_mutex_lock(&mutexLogFile);
                fprintf(stdout,"| ID CASSA: %d | N. PROD. ELAB.: %d | N. Clienti: %d | TEMPO TOT: %.3f s | TEMPO MEDIO: %.3f s | N. CHIUSURE: %d |\n",((cassiere *)arg)->id, numProdElab, numClientiElab, tempoTot, tempoTot/numClientiElab, ((cassiere *)arg)->numeroChiusure);
                fflush(stdout);
                pthread_mutex_unlock(&mutexLogFile);
            }
            pthread_mutex_unlock(&mutexCodeCassieri[((cassiere *)arg)->id - 1]);
        }else{
            pthread_mutex_lock(&mutexFlagCodeVuoteCassieri[((cassiere *)arg)->id - 1]);
            codeCassieriVuote[((cassiere *)arg)->id - 1] = 1;
        }
        pthread_mutex_unlock(&mutexCodeCassieri[((cassiere *)arg)->id - 1]);
        pthread_cond_signal(&condFlagCodeVuoteCassieri[((cassiere *)arg)->id - 1]);
        pthread_mutex_unlock(&mutexFlagCodeVuoteCassieri[((cassiere *)arg)->id - 1]);
    }
    return NULL;
}

void *DirettoreClienti(void *arg){
    client *clientiUscenti = malloc(sizeof(client)*cfg.E);
    int k=0;
    while(1){
        //se la coda è vuota si mette in attesa
        pthread_mutex_lock(&mutexFlagCodaVuotaDirettore);
        while(codaDirettoreVuota){
            pthread_cond_wait(&condFlagCodaVuotaDirettore,&mutexFlagCodaVuotaDirettore);
        }
        pthread_mutex_unlock(&mutexFlagCodaVuotaDirettore);
        //Se la coda non è vuota il direttore fa il lock, perchè la deve svuotare pian piano
        pthread_mutex_lock(&mutexCodaDirettore);
        if(!isEmpty(*codaDirettore)){
            //Se la coda non è vuota, prendo il cliente attuale e lo metto nell'array,
            //per preservare il suo id
            //Quindi se la coda non è vuota, prendo il cliente attuale:
            client attuale = (codaDirettore->head)->c;
            //lo metto nell'array degli uscenti
            clientiUscenti[k].id = attuale.id;
            clientiUscenti[k].numCodeViste = attuale.numCodeViste;
            //incremento k
            k++;
            //tolgo il cliente dalla coda
            dequeue(codaDirettore);
            pthread_mutex_unlock(&mutexCodaDirettore);
            if(k==cfg.E){
                client *cl = malloc(sizeof(client)*k);
                //Genero cfg->C clienti
                for(int i=0;i<k;i++){
                    //DEVO RIESEGUIRE LE PROCEDURE DI CLIENTE*
                    //Assegno il numero di prodotti da acquistare in modo casuale (da 0 a P)
                    unsigned int seed1 = clientiUscenti[i].id + time(NULL);
                    long r = generaProdotto(seed1);
                    cl[i].id = clientiUscenti[i].id;
                    cl[i].numProd = (int)r;
                    //Assegno il tempo per acquistare i prodotti (da 10 a T)
                    unsigned int seed2 = cl[i].id + time(NULL);
                    r = generaTempoAcquisto(seed2);
                    cl[i].tempoAcquisto = (float)r/1000;
                    long nanoSec = cl[i].tempoAcquisto * 1000000000;
                    //creo la struttura per i nanosecondi:
                    struct timespec t = {0, nanoSec};
                    nanosleep(&t,NULL);
                    if(cl[i].numProd == 0){
                        //direttore
                        //penso alla coda
                        pthread_mutex_lock(&mutexCodaDirettore);
                        enqueue(codaDirettore,cl[i]);
                        pthread_mutex_unlock(&mutexCodaDirettore);
                        //penso al flag
                        pthread_mutex_lock(&mutexFlagCodaVuotaDirettore);
                        codaDirettoreVuota = 0;
                        pthread_cond_signal(&condFlagCodaVuotaDirettore);
                        pthread_mutex_unlock(&mutexFlagCodaVuotaDirettore);
                    }else{
                        //cassiere
                        //Va scelto il cassiere:
                        //Conto le casse aperte:
                        //Per accedere a cassieri che ora è globale devo farlo in mutua esclusione:
                        cl[i].numCodeViste++;
                        pthread_mutex_lock(&mutexCassieri);
                        int max = contaCasseAperte(cassieri);
                        //genero un array con sole le casse aperte
                        cassiere *casseOpen = casseAperte(cassieri, max);
                        pthread_mutex_unlock(&mutexCassieri);
                        
                        //Ora mi creo un seed
                        unsigned int seed3 = cl[i].id + time(NULL);
                        long cas = rand_r(&seed3)%(max);
                        //Penso alla coda scelta
                        pthread_mutex_lock(&mutexCodeCassieri[casseOpen[cas].id]);
                        enqueue(codeCassieri[casseOpen[cas].id],cl[i]);
                        pthread_mutex_unlock(&mutexCodeCassieri[casseOpen[cas].id]);
                        //penso al flag
                        pthread_mutex_lock(&mutexFlagCodeVuoteCassieri[casseOpen[cas].id]);
                        codeCassieriVuote[casseOpen[cas].id] = 0;
                        pthread_cond_signal(&condFlagCodeVuoteCassieri[casseOpen[cas].id]);
                        pthread_mutex_unlock(&mutexFlagCodeVuoteCassieri[casseOpen[cas].id]);
                    }
                }
                k=0;
            }
        }else{
            pthread_mutex_lock(&mutexFlagCodaVuotaDirettore);
            codaDirettoreVuota = 1;
        }
        pthread_mutex_unlock(&mutexCodaDirettore);
        pthread_cond_signal(&condFlagCodaVuotaDirettore);
        pthread_mutex_unlock(&mutexFlagCodaVuotaDirettore);

    }
    return NULL;
}

void *TimerCassa(void *arg){
    while(1){
         //Se chiusa, wait fino a che Direttore non dice di aprire (wait)
        pthread_mutex_lock(&mutexCasseChiuse[((cassiere *)arg)->id - 1]);
        while(casseChiuse[((cassiere *)arg)->id - 1]){
            pthread_cond_wait(&condCasseChiuse[((cassiere *)arg)->id - 1],&mutexCasseChiuse[((cassiere *)arg)->id - 1]);
        }
        pthread_mutex_unlock(&mutexCasseChiuse[((cassiere *)arg)->id - 1]);
        //La cassa ha aperto, devo attendere cfg.tempo_info millisecondi per poi avvisare il direttore
        long r = cfg.tempo_info * 1000000;
        struct timespec t = {0,r};
        nanosleep(&t,NULL);
        //Ora devo avvisare il direttore
        //Prima penso ad aggiornare i dati
        //Prima locko l'array con le info da aggiornare
        //Poi locko la coda da cui leggere
        pthread_mutex_lock(&mutexInfoCasse[((cassiere *)arg)->id - 1]);
        pthread_mutex_lock(&mutexCodeCassieri[((cassiere *)arg)->id - 1]);
        infoCasse[((cassiere *)arg)->id - 1] = codeCassieri[((cassiere *)arg)->id - 1]->dim;
        pthread_mutex_unlock(&mutexCodeCassieri[((cassiere *)arg)->id - 1]);
        pthread_mutex_unlock(&mutexInfoCasse[((cassiere *)arg)->id - 1]);
        //Ora penso al segnale
        pthread_mutex_lock(&mutexFlagInfoCasse);
        flagInfoCasse = ((cassiere *)arg)->id - 1;
        pthread_cond_signal(&condFlagInfoCasse);
        pthread_mutex_unlock(&mutexFlagInfoCasse);
    }
}

void *DirettoreCasse(void *arg){
    while(1){
        int a = -1;
        int info = 0;
        //Attendo che mi arrivi un segnale
        pthread_mutex_lock(&mutexFlagInfoCasse);
        while(flagInfoCasse == -1){
            pthread_cond_wait(&condFlagInfoCasse,&mutexFlagInfoCasse);
        }

        a = flagInfoCasse;
        pthread_mutex_unlock(&mutexFlagInfoCasse);
        //a è l'indice della cassa che ha mandato il segnale
        //quindi posso lockare infoCasse[a] e prelevare il numero, stampandolo
        pthread_mutex_lock(&mutexInfoCasse[a]);
        info = infoCasse[a];
        pthread_mutex_unlock(&mutexInfoCasse[a]);
        //Se ci sono almeno S1 casse con info <=1, chiudo una cassa
        int esse1 = 0;
        pthread_mutex_lock(&mutexCassieri);
        for(int i=0;i<cfg.K;i++){
            //scorro i cassieri che sono aperti
            pthread_mutex_lock(&mutexInfoCasse[i]);
            if(cassieri[i].cassaAperta == 1 && info <=1){
                esse1++;
            }
            pthread_mutex_unlock(&mutexInfoCasse[i]);
        }
        pthread_mutex_unlock(&mutexCassieri);

        //Se ci sono almeno info=S2 in una cassa, ne apro una (se cAperte è minore di cfg.K)
        //Quindi scorro tutte le casse
        pthread_mutex_lock(&mutexCassieri);
        int cAperte = contaCasseAperte(cassieri);
        pthread_mutex_unlock(&mutexCassieri);
        int vero=0;
        for(int i=0;i<cfg.K;i++){
            pthread_mutex_lock(&mutexInfoCasse[i]);
            if(infoCasse[i]<=cfg.S2 && cAperte<cfg.K){
                //Allora esco da questo ciclo e ne apro una
                vero=1;
            }
            pthread_mutex_unlock(&mutexInfoCasse[i]);
        }
        if(esse1>=cfg.S1){
            pthread_mutex_lock(&mutexCassieri);
            int max = contaCasseAperte(cassieri);
            pthread_mutex_unlock(&mutexCassieri);
            int *aperte = malloc(sizeof(int)*max);
            int k=0;
            for(int i=0;i<cfg.K;i++){
                pthread_mutex_lock(&mutexCasseChiuse[i]);
                if(casseChiuse[i] == 0){
                    aperte[k] = i;
                    k++;
                }
                pthread_mutex_unlock(&mutexCasseChiuse[i]);
            }
            unsigned int seed = *(int *)arg + time(NULL);
            int r = (int)rand_r(&seed)%(max);
            pthread_mutex_lock(&mutexCassieri);
            chiudiCassa(&cassieri[aperte[r]]);
            pthread_mutex_unlock(&mutexCassieri);
            pthread_mutex_lock(&mutexCasseChiuse[aperte[r]]);
            casseChiuse[aperte[r]] = 1;
            pthread_cond_signal(&condCasseChiuse[aperte[r]]);
            pthread_mutex_unlock(&mutexCasseChiuse[aperte[r]]);
            //cDaChiudere = (cDaChiudere + *(int *)arg)%cAperte -1;
            //Chiudo la cassa
            //Aggiorno l'id
            *(int *)arg = (*(int *)arg +7)%191;
            free(aperte);
        } 
        if(vero){
        //se sono uscito perchè vero è 1, devo aprire una cassa tra quelle chiuse, random:
        //Devo scegliere tra le casse chiuse: so che ci sono cAperte casse aperte, quindi ho cfg.K-cAperte casse chiuse:
            pthread_mutex_lock(&mutexCassieri);
            int max = cfg.K-contaCasseAperte(cassieri);
            pthread_mutex_unlock(&mutexCassieri);
            int *chiuse = malloc(sizeof(int)*max);
            int k=0;
            for(int i=0;i<cfg.K;i++){
                pthread_mutex_lock(&mutexCasseChiuse[i]);
                if(casseChiuse[i] == 1){
                    chiuse[k] = i;
                    k++;
                }
                pthread_mutex_unlock(&mutexCasseChiuse[i]);
            }
            //Ora ho salvato gli indici delle casse chiuse in un array da 0 a max-1
            //Posso scegliere un numero random tra 0 e max-1 che corrisponderà alla 
            //cassa casuale da aprire:
            unsigned int seed = *(int *)arg + time(NULL);
            int r = (int)rand_r(&seed)%(max);
            *(int *)arg = (*(int *)arg +3)%79;
            pthread_mutex_lock(&mutexCassieri);
            apriCassa(&cassieri[chiuse[r]]);
            pthread_mutex_unlock(&mutexCassieri);
            pthread_mutex_lock(&mutexCasseChiuse[chiuse[r]]);
            casseChiuse[chiuse[r]] = 0;
            pthread_cond_signal(&condCasseChiuse[chiuse[r]]);
            pthread_mutex_unlock(&mutexCasseChiuse[chiuse[r]]);
            free(chiuse);
        }
        pthread_mutex_lock(&mutexFlagInfoCasse);
        flagInfoCasse = -1;
        //forse non servo io!
        pthread_cond_signal(&condFlagInfoCasse);
        pthread_mutex_unlock(&mutexFlagInfoCasse);
    }
}
void handler(int signum){
    if(signum == 3){
        //SIGQUIT
        fprintf(stdout,"RICEVUTO SEGNALE SIGQUIT\n");
        exit(EXIT_SUCCESS);
    }else if(signum == 1){
        //SIGHUP
        fprintf(stdout,"RICEVUTO SEGNALE SIGHUP\n");
        exit(EXIT_SUCCESS);
    }
}

void inizializzaConfig(){
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    fp = fopen("config.txt", "r");
     if (fp == NULL)
        exit(EXIT_FAILURE);
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
    fclose(fp);
    if (line)
        free(line);
    cfg.tempo_info = 20;
    cfg.casse_iniziali = 1;
    cfg.num_clienti = 0;
    cfg.num_prodotti = 0;
}

void initCassieri(){
    //mi preparo ad avere k code
    codeCassieri = malloc(sizeof(coda*)*cfg.K);
    //mi preparo ad avere k mutex/cv/flag
    mutexCodeCassieri = malloc(sizeof(pthread_mutex_t)*cfg.K);
    codeCassieriVuote = malloc(sizeof(int)*cfg.K);
    mutexFlagCodeVuoteCassieri = malloc(sizeof(pthread_mutex_t)*cfg.K);
    condFlagCodeVuoteCassieri = malloc(sizeof(pthread_cond_t)*cfg.K);
    casseChiuse = malloc(sizeof(int)*cfg.K);
    mutexCasseChiuse = malloc(sizeof(pthread_mutex_t)*cfg.K);
    condCasseChiuse = malloc(sizeof(pthread_cond_t)*cfg.K);
    mutexInfoCasse = malloc(sizeof(pthread_mutex_t)*cfg.K);
    infoCasse = malloc(sizeof(int)*cfg.K);
    flagInfoCasse = -1;
    condCodeCassieri = malloc(sizeof(pthread_cond_t)*cfg.K);
    flagCodeCassieri = malloc(sizeof(int)*cfg.K);
    for(int i=0;i<cfg.K;i++){
        //alloco k code vuote
        codeCassieri[i] = creaCoda();
        //Inizializzo le mutex/cv/flags
        pthread_mutex_init(&mutexCodeCassieri[i],NULL);
        codeCassieriVuote[i] = 1;
        pthread_mutex_init(&mutexFlagCodeVuoteCassieri[i],NULL);
        pthread_cond_init(&condFlagCodeVuoteCassieri[i],NULL);
        casseChiuse[i] = 1;
        if(i < cfg.casse_iniziali){
            casseChiuse[i] = 0;
        }
        pthread_mutex_init(&mutexCasseChiuse[i],NULL);
        pthread_cond_init(&condCasseChiuse[i],NULL);
        pthread_mutex_init(&mutexInfoCasse[i],NULL);
        infoCasse[i]=0;
        pthread_cond_init(&condCodeCassieri[i],NULL);
        flagCodeCassieri[i] = 1;
    }
}