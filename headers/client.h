typedef struct client{
    int id;
    int numProd; //prodotti acquistati
    float tempoCoda; //tempo speso in coda
    float tempoAcquisto; //tempo per acquistare
    int numCodeViste; //numero di code visitate
    struct timespec entry_time;
    struct timespec exit_time;
}client;

void inizializzaCliente(client *c, int id){
    c->id = ++id;
    c->numProd = 0;
    c->tempoCoda = 0;
    c->tempoAcquisto = 0;
    c->numCodeViste = 0;
    c->entry_time.tv_sec = 0;
    c->entry_time.tv_nsec =0;
    c->exit_time.tv_nsec=0;
    c->exit_time.tv_sec=0;
}

//Funzione che genera un numero casuale con seed da 0 a P
long generaProdotto(unsigned int seed, int numeroProdotti){
    long r = rand_r(&seed)% (numeroProdotti + 1);
    return r;
}
//Funzione che genera un tempo casuale per l'acquisto tra 10 e T
long generaTempoAcquisto(unsigned int seed, int tempoAcquisto){
    long r = rand_r(&seed)% (tempoAcquisto + 1 - 10) + 10;
    return r;
}

long generaCassiere(unsigned int *seed, int max){
    long r = rand_r(seed)%(max);
    return r;
}

//Funzioni per test
void printCliente(client c){
    fprintf(stdout,"ID_CLIENTE: %d\nNUMERO_PROD: %d\nTEMPO_IN_CODA: %.3f\nNUM_CODE_VISTE: %d\n",c.id,c.numProd,c.tempoCoda,c.numCodeViste);
    fflush(stdout);
}