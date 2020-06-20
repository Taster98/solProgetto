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

//Funzione per il thread Cliente
void *Cliente(void *arg){
    while(!sighup && !sigquit){
        
    }
}