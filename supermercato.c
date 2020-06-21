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
static void inizializzaConfig();
//testing config
//static void testConfig();
//Variabile globale per il file di configurazione
static config cfg;
//Variabili globali per gestire i segnali SIGHUP e SIGQUIT
volatile sig_atomic_t sighup = 0;
volatile sig_atomic_t sigquit = 0;

static void handler(int signum);


//file aggiuntivi
#include "./headers/client.h"
#include "./headers/cassiere.h"
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

//CODA PER CASSIERE
coda *codaCassiere;
pthread_mutex_t mutexCodaCassiere = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condCodaCassiere = PTHREAD_COND_INITIALIZER;
int codaCassiereVuota = 1;
//Mi serve una mutex/cond anche per il flag, poichè sarà poi il cliente a settarla a 0
pthread_mutex_t mutexFlagCodaVuotaCassiere = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condFlagCodaVuotaCassiere = PTHREAD_COND_INITIALIZER;

//Funzione per il thread Cliente
void *Cliente(void *arg);
//Funzione per il thread Cassiere
void *Cassiere(void *arg);

int main(){
    //INIZIALIZZO GESTORE SEGNALI
    struct sigaction s;
    memset(&s,0,sizeof(s));
    s.sa_handler=handler;
    ec_meno1(sigaction(SIGHUP,&s,NULL),"Errore segnale di SIGHUP");
    ec_meno1(sigaction(SIGQUIT,&s,NULL),"Errore segnale di SIGQUIT");
    //inizializzo il config (che poi leggerà dal file config.txt)
    inizializzaConfig();
    //inizializzo le code
    codaDirettore = creaCoda();
    codaCassiere = creaCoda();
    //Genero cfg->C clienti
    client *cl = malloc(sizeof(client)*cfg.C);
    pthread_t *thids = malloc(sizeof(pthread_t)*cfg.C);
    for(int i=0;i<cfg.C;i++){
        thids[i] = i;
        inizializzaCliente(&cl[i],i);
        if(pthread_create(&thids[i],NULL,&Cliente,&cl[i]) != 0){
            fprintf(stderr,"Errore creazione thread %d-esimo",i);
            exit(EXIT_FAILURE);
        }
    }
    //GENERO IL CASSIERE
    pthread_t id_cass = 1;
    cassiere cass;
    inizializzaCassiere(&cass,0);
    if(pthread_create(&id_cass,NULL,&Cassiere,&cass) != 0){
            fprintf(stderr,"Errore creazione thread cassiere");
            exit(EXIT_FAILURE);
    }
    //GENERO IL DIRETTORE

    //QUI AVVERRÀ IL PROGRAMMA
    while(sighup == 0 && sigquit == 0){
        break;
    }

    //JOIN CLIENTI
    for(int i=0;i<cfg.C;i++){
        int status;
        if(pthread_join(thids[i],(void *)&status) != 0){
            fprintf(stderr,"Errore join thread %d-esimo",i);
            exit(EXIT_FAILURE);
        }
    }
    //JOIN CASSIERE
    if(pthread_join(id_cass,NULL) != 0){
            fprintf(stderr,"Errore join thread cassiere");
            exit(EXIT_FAILURE);
    }
    //JOIN DIRETTORE
    free(thids);
    fprintf(stdout,"\n\nFinito, ho creato %d thread\n",cfg.C);
    return 0;
}

//Funzione per il thread Cliente
void *Cliente(void *arg){
    
    //fprintf(stdout,"CI SONOOOO\n");
    //Assegno il numero di prodotti da acquistare in modo casuale (da 0 a P)
    unsigned int seed1 = ((client *)arg)->id + time(NULL);
    long r = generaProdotto(seed1);
    ((client *)arg)->numProd = (int)r;
    //Assegno il tempo per acquistare i prodotti (da 10 a T)
    unsigned int seed2 = ((client *)arg)->id + time(NULL);
    r = generaTempoAcquisto(seed2);
    ((client *)arg)->tempoAcquisto = (float)r/1000;

    fprintf(stderr,"Il cliente %d acquisterà %d prodotti in %.3f secondi\n",((client *)arg)->id, ((client *)arg)->numProd,((client *)arg)->tempoAcquisto);
    fflush(stdout);
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
        //Penso alla coda
        pthread_mutex_lock(&mutexCodaCassiere);
        enqueue(codaCassiere,*((client *)arg));
        pthread_mutex_unlock(&mutexCodaCassiere);
        //penso al flag
        pthread_mutex_lock(&mutexFlagCodaVuotaCassiere);
        codaCassiereVuota = 0;
        pthread_cond_signal(&condFlagCodaVuotaCassiere);
        pthread_mutex_unlock(&mutexFlagCodaVuotaCassiere);
    }
    

    return NULL;
}

void *Cassiere(void *arg){
    while(1){
        pthread_mutex_lock(&mutexFlagCodaVuotaCassiere);
        //finchè la coda è vuota non faccio nulla
        while(codaCassiereVuota){
            pthread_cond_wait(&condFlagCodaVuotaCassiere,&mutexFlagCodaVuotaCassiere);
        }
        //Se la coda non è vuota, allora attendo il tempo costante random del cassiere
        //converto il tempo fisso in nanosecondi
        long nanoSec = ((cassiere *)arg)->tempoFisso * 1000000000;
        struct timespec t = {0, nanoSec};
        nanosleep(&t,NULL);
        //ora posso lockare la coda, e iniziare a servire cliente dopo cliente
        pthread_mutex_lock(&mutexCodaCassiere);
        if(!isEmpty(*codaCassiere)){
            //prendo il cliente attuale
            client attuale = (codaCassiere->head)->c;
            //converto in nanosecondi il tempo prodotto
            long nanoSec2 = ((cassiere *)arg)->tempoProdotto * 1000000000;
            struct timespec t2 = {0,nanoSec2};
            //PRINT DI TEST
            fprintf(stdout,"Servendo il cliente numero %d\n",attuale.id);
            fflush(stdout);
            //Ora attendo ((cassiere *)arg)->tempoProdotto per un ciclo che va da 0 a attuale->numProd
            for(int i=0;i<attuale.numProd;i++){
                nanosleep(&t2,NULL);
            }
            fprintf(stdout,"Cliente numero %d SERVITO\n",attuale.id);
            fflush(stdout);
            dequeue(codaCassiere);
        }else{
            codaCassiereVuota = 1;
        }
        pthread_cond_signal(&condFlagCodaVuotaCassiere);
        pthread_mutex_unlock(&mutexCodaCassiere);
        pthread_mutex_unlock(&mutexFlagCodaVuotaCassiere);
    }
    return NULL;
}

static void handler(int signum){
    if(signum == 1){
        //SIGHUP
        fprintf(stdout,"RICEVUTO SEGNALE SIGHUP");
        exit(EXIT_SUCCESS);
    }else if(signum == 3){
        //SIGQUIT
        fprintf(stdout,"RICEVUTO SEGNALE SIGQUIT");
        exit(EXIT_SUCCESS);
    }
}

static void inizializzaConfig(){
    cfg.K = 6;
    cfg.C = 50;
    cfg.E = 3;
    cfg.T= 200;
    cfg.P= 100;
    cfg.S= 20;
    cfg.S1= 2;
    cfg.S2= 10;
    cfg.tempo_info = 35;
    cfg.casse_iniziali = 1;
    cfg.num_clienti = 0;
    cfg.num_prodotti = 0;
}

/*static void testConfig(){
    printf("Casse max: %d\nClienti max: %d\nClienti entranti/uscenti max: %d\nTempo max: %d\n",cfg.K,cfg.C,cfg.E,cfg.T);
}*/

