typedef struct cassiere{
    int id; //id del cassiere
    int numClients; //numero clienti serviti
    int numProd; //numero prodotti elaborati
    float tempoFisso; //tempo fisso random
    float tempoProdotto; //tempo dipendente dal numero di prodotti, dal cfg
    float tempoMedioCliente; //tempo medio per servire un cliente
    float tempoApertura; //tempo di apertura della cassa
    int cassaAperta; //la cassa è aperta o chiusa?
    int numeroChiusure; //quante volte la cassa ha chiuso
}cassiere;

long randomFisso(unsigned int seed);

void inizializzaCassiere(cassiere *c,int id){
    c->id = ++id;
    c->numClients = 0;
    c->numProd = 0;
    //calcolo i millisecondi
    unsigned seed = (unsigned)c->id + time(NULL);
    long r = randomFisso(seed);
    //tempo convertito in secondi
    c->tempoFisso = (float)r/1000;
    c->tempoProdotto = cfg.S/1000;
    c->tempoMedioCliente = 0;
    c->tempoApertura = 0;
    c->cassaAperta = 1;
    c->numeroChiusure =0;
}

void apriCassa(cassiere *c){
    c->cassaAperta = 1;
}

void chiudiCassa(cassiere *c){
    //STAMPA TUTTE LE INFO:
    //| ID_CASSA | N_PROD_ELABORATI | N_CLIENTI | TEMPO_TOT_APERTURA | TEMPO_MEDIO | N_CHIUSURE |
    c->cassaAperta = 0;
    ++c->numeroChiusure;
    c->numClients = 0;
    c->numProd = 0;
    c->tempoMedioCliente = 0;
    c->tempoApertura = 0;
}

//funzioni ausiliarie
long randomFisso(unsigned int seed){
    long r = rand_r(&seed)% (T_MAX_CASS + 1 - T_MIN_CASS) + T_MIN_CASS;
    return r;
}

void printCassiere(cassiere c){
    fprintf(stdout,"| id cassa | n. prodotti elaborati | n. di clienti | tempo tot. di apertura | tempo medio di servizio | n. chiusure |\n");
    fflush(stdout);
    fprintf(stdout,"|    %d     |           %d           |       %d       |         %.3f          |          %.3f          |      %d      |\n",c.id,c.numProd,c.numClients,c.tempoApertura,c.tempoMedioCliente,c.numeroChiusure);
    fflush(stdout);
}