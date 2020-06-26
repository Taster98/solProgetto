typedef struct client{
    int id;
    int numProd; //prodotti acquistati
    float tempoSuper; //tempo totale supermercato
    float tempoCoda; //tempo speso in coda
    float tempoAcquisto; //tempo per acquistare
    int numCodeViste; //numero di code visitate
    int cassaUsata; //la cassa che il cliente utilizza
}client;

void inizializzaCliente(client *c, int id){
    c->id = ++id;
    c->numProd = 0;
    c->tempoSuper = 0;
    c->tempoCoda = 0;
    c->tempoAcquisto = 0;
    c->numCodeViste = 0;
    c->cassaUsata = 0;
}

void printCliente(client c){
    fprintf(stdout,"ID_CLIENTE: %d\nNUMERO_PROD: %d\nTEMPO_NEL_SM: %.3f\nTEMPO_IN_CODA: %.3f\nNUM_CODE_VISTE: %d\n",c.id,c.numProd,c.tempoSuper,c.tempoCoda,c.numCodeViste);
    fflush(stdout);
}
//Funzione che genera un numero casuale con seed da 0 a P
long generaProdotto(unsigned int seed, int numeroProdotti){
    long r = rand_r(&seed)% (numeroProdotti + 1);
    //fprintf(stdout,"Ciao %lu\n",r);
    return r;
}
//Funzione che genera un tempo casuale per l'acquisto tra 10 e T
long generaTempoAcquisto(unsigned int seed, int tempoAcquisto){
    long r = rand_r(&seed)% (tempoAcquisto + 1 - 10) + 10;
    //fprintf(stdout,"Ciao Acquisto %lu\n",r);
    return r;
}

long generaCassiere(unsigned int *seed, int max){
    long r = rand_r(seed)%(max);
    return r;
}