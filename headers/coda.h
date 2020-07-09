typedef struct elem{
    client c; //cliente singolo
    struct elem *next; //cliente successivo
}elem;

typedef struct coda{
    elem *head;
    elem *tail;
    client *clients; //insieme di clienti
    int dim; //numero clienti in coda
}coda;

//funzione che crea un elemento da inserire in coda
elem *creaElem(client tmp){
    elem *nuovo = (elem*)malloc(sizeof(elem));
    nuovo->c = tmp;
    nuovo->next = NULL;
    return nuovo;
}

//funzione che crea una coda vuota
coda *creaCoda(){
    coda *q = (coda*)malloc(sizeof(coda));
    q->head = q->tail = NULL;
    q->dim=0;
    return q;
}

//funzione che inserisce tmp in coda q
void enqueue(coda *q, client tmp){
    elem *e;
    e = creaElem(tmp);
    //se la coda è vuota aggiungo sia in testa che in coda:
    if(q->tail == NULL){
        q->head=q->tail=e;
    }else{
        //altrimenti devo aggiungere in fondo e "spostare" l'indice che punta alla coda
        q->tail->next = e;
        q->tail = e;
    }
    //incremento la dimensione
    q->dim++;
}

int isEmpty(coda q){
    if(q.head == NULL && q.tail == NULL){
        return 1;
    }else{
        return 0;
    }
}

//funzione che estrae dalla coda e ritorna l'elemento in testa
void dequeue(coda *q){
    if(q->head != NULL){
        elem *tmp = q->head;
        //scala di un posto la testa
        q->head = q->head->next;
        //se siamo arrivati in fondo inizializzo anche la coda
        if(q->head == NULL)
            q->tail = NULL;
        free(tmp);
        //decremento la dimensione
        q->dim--;
    }
}
//Funzione ausiliaria per aggiornare il tempo in coda dei clienti
void aggiornaTempoCoda(coda q, float tempoFisso, float tempoVar){
    elem *corr = q.head;
    float t = 0;
    while(corr != NULL){
        t = t + tempoFisso + (corr->c.numProd * tempoVar);
        corr->c.tempoCoda = t;
        corr = corr->next;
    }
}
//funzione che stampa la coda
void printQueue(coda q){
    elem *corr = q.head;
    printf("Stampo la coda:\n");
    while(corr != NULL){
        printCliente(corr->c);
        printf("\n");
        corr = corr->next;
    }
}
