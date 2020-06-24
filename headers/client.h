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

long generaCassiere(unsigned int *seed, int max){
    long r = rand_r(seed)%(max);
    return r;
}