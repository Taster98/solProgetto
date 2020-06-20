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
/*QUI DEFINISCO LE VARIE MUTEX STATICHE*/

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
//coda globale acceduta da chi non acquista niente
//coda *coda_uscenti;
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


//file aggiuntivi
#include "./headers/client.h"
#include "./headers/cassiere.h"
#include "./errors/error.h"
#include "./headers/coda.h"

//Coda dei non compranti
coda *nonCompranti;
pthread_mutex_t mutex_non_compranti = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_non_compranti = PTHREAD_COND_INITIALIZER;

int main(){
    //GESTORE
    struct sigaction s;
    nonCompranti = creaCoda(); 
    memset(&s,0,sizeof(s));
    s.sa_handler=handler;
    ec_meno1(sigaction(SIGHUP,&s,NULL),"Errore segnale di SIGHUP");
    ec_meno1(sigaction(SIGQUIT,&s,NULL),"Errore segnale di SIGQUIT");
    //inizializzo il config
    inizializzaConfig();
    //testConfig();
    //test random
    //inizializzo cassiere
    cassiere c;
    inizializzaCassiere(&c,0);
    //printCassiere(c);

    //TEST genero cfg->C clienti
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
    //JOIN
    for(int i=0;i<cfg.C;i++){
        int status;
        if(pthread_join(thids[i],(void *)&status) != 0){
            fprintf(stderr,"Errore join thread %d-esimo",i);
            exit(EXIT_FAILURE);
        }
    }
    free(thids);
    fprintf(stdout,"\n\nFinito, ho creato %d thread\n",cfg.C);
    return 0;
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

