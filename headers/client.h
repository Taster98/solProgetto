typedef struct client{
    int id;
    int numProd; //prodotti acquistati
    float tempoSuper; //tempo totale supermercato
    float tempoCoda; //tempo speso in coda
    float tempoAcquisto; //tempo per acquistare
    int numCodeViste; //numero di code visitate
    int cassaUsata; //la cassa che il cliente utilizza
    //Il cliente avrà una mutex relativa alla coda della cassa utilizzata.
    pthread_mutex_t mutex_coda;
    //Il cliente ha anche una variabile di condizione per segnalare quando uscirà dalla coda
    pthread_cond_t cond_coda;
    
}client;

void inizializzaCliente(client *c, int id){
    c->id = ++id;
    c->numProd = 0;
    c->tempoSuper = 0;
    c->tempoCoda = 0;
    c->tempoAcquisto = 0;
    c->numCodeViste = 0;
    c->cassaUsata = 0;
    //inizializzo la mutex
    if(pthread_mutex_init(&c->mutex_coda, NULL) != 0){
        fprintf(stderr,"Errore inizializzazione mutex\n");
        exit(EXIT_FAILURE);
    }
    //inizializzo la vc
    if(pthread_cond_init(&c->cond_coda, NULL) != 0){
        fprintf(stderr,"Errore inizializzazione VC\n");
        exit(EXIT_FAILURE);
    }
}

void printCliente(client c){
    printf("ID_CLIENTE: %d\nNUMERO_PROD: %d\nTEMPO_NEL_SM: %.3f\nTEMPO_IN_CODA: %.3f\nNUM_CODE_VISTE: %d\n",c.id,c.numProd,c.tempoSuper,c.tempoCoda,c.numCodeViste);
}
//Funzione che genera un numero casuale con seed da 0 a P
long generaProdotto(unsigned int seed){
    long r = rand_r(&seed)% (cfg.P + 1);
    //fprintf(stdout,"Ciao %lu\n",r);
    return r;
}
//Funzione che genera un tempo casuale per l'acquisto tra 10 e T
long generaTempoAcquisto(unsigned int seed){
    long r = rand_r(&seed)% (cfg.T + 1 - 10) + 10;
    //fprintf(stdout,"Ciao Acquisto %lu\n",r);
    return r;
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

    //In caso in cui ((client *)arg)->numProd è uguale a 0:
    

    return NULL;
}