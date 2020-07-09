typedef struct cassiere{
    int id; //id del cassiere
    int numClients; //numero clienti serviti
    int numProd; //numero prodotti elaborati
    float tempoFisso; //tempo fisso random
    float tempoProdotto; //tempo dipendente dal numero di prodotti, dal cfg
    float tempoApertura; //tempo totale di apertura della cassa
    int cassaAperta; //la cassa è aperta o chiusa?
    int numeroChiusure; //quante volte la cassa ha chiuso
}cassiere;

long randomFisso(unsigned int seed);

void inizializzaCassiere(cassiere *c,int id, float tempoProdotto){
    c->id = ++id;
    c->numClients = 0;
    c->numProd = 0;
    //calcolo i millisecondi
    unsigned seed = (unsigned)c->id + time(NULL);
    long r = randomFisso(seed);
    //tempo convertito in secondi
    c->tempoFisso = (float)r;
    c->tempoProdotto = tempoProdotto;
    c->tempoApertura = 0;
    c->cassaAperta = 0;
    c->numeroChiusure =0;
}

void apriCassa(cassiere *c){
    //if(c->cassaAperta != -1)
        c->cassaAperta = 1;
}

void chiudiCassa(cassiere *c){
    c->cassaAperta = 0;
    (c->numeroChiusure)++;
}
//funzioni ausiliarie
long randomFisso(unsigned int seed){
    long r = rand_r(&seed)% (T_MAX_CASS + 1 - T_MIN_CASS) + T_MIN_CASS;
    return r;
}
//Funzioni per test
void printCassiere(cassiere c){
    fprintf(stdout,"| id cassa | n. prodotti elaborati | n. di clienti | tempo tot. di apertura | tempo medio di servizio | n. chiusure |\n");
    fflush(stdout);
    fprintf(stdout,"|    %d     |           %d           |       %d       |         %.3f          |          0          |      %d      |\n",c.id,c.numProd,c.numClients,c.tempoApertura,c.numeroChiusure);
    fflush(stdout);
}