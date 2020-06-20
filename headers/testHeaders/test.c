#include<stdio.h>
#include<stdlib.h>
#include "../client.h"
#include "../coda.h"

void testaCliente(){
    client a;
    int id = 0;
    a = creaCliente(id);
    printCliente(a);
}
void testaCoda(){
    //creo una coda vuota
    coda *codaVuota = creaCoda();
    //aggiungo un cliente in coda con id=3
    int id=2;
    client c1 = creaCliente(id);
    //aggiungo un cliente in coda con id=4
    id=3;
    client c2 = creaCliente(id);
    //aggiungo un cliente in coda con id=5
    id=4;
    client c3 = creaCliente(id);
    //aggiungo elemento in coda
    enqueue(codaVuota,c1);
    //aggiungo elemento in coda
    enqueue(codaVuota,c2);
    //aggiungo elemento in coda
    enqueue(codaVuota,c3);
    //stampo coda
    printQueue(*codaVuota);
    printf("Elimino 2 clienti e ristampo\n\n");
    dequeue(codaVuota);
    dequeue(codaVuota);
    printQueue(*codaVuota);
}

int main(){
    //testaCliente();
    testaCoda();
    return 0;
}
