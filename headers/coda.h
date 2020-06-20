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
    return q;
}

//funzione che inserisce in coda
//prendo il cliente tmp da inserire in coda q
void enqueue(coda *q, client tmp){
    elem *e = creaElem(tmp);

    //se la coda è vuota aggiungo sia in testa che in coda:
    if(q->tail == NULL){
        q->head=q->tail=e;
        return;
    }
    //altrimenti devo aggiungere in fondo e "spostare" l'indice che punta alla coda
    q->tail->next = e;
    q->tail = e;
}


//funzione che estrae dalla coda
void dequeue(coda *q){
    if(q->head == NULL)
        return;
    elem *tmp = q->head;
    //scala di un posto la testa
    q->head = q->head->next;
    //se siamo arrivati in fondo inizializzo anche la coda
    if(q->head == NULL)
        q->tail = NULL;

    free(tmp);
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
