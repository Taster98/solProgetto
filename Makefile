#inizio makefile
#scelgo il compilatore
CC = gcc

#aggiungo eventuali flag standard (-g -pedantic e -Wall)
CFLAGS = -g -pedantic -Wall

#aggiungo la libreria usata per i thread
LIBLINK = -lpthread

#file di configurazione standard
CONFIG = ./config/config.txt

#definisco i "falsi" target
.PHONY: clean test

main: ./supermercato

./supermercato: ./supermercato.c
	$(CC) $@.c $(CFLAGS) -o $@ $(LIBLINK)

test:
	(./supermercato -c $(CONFIG) & echo $$! > supermercato.PID) &
	sleep 25s; \
	kill -s 1 $$(cat supermercato.PID); \
	chmod +x ./analisi.sh
	./analisi.sh $$(cat supermercato.PID);\

clean: 
	echo Pulizia generale...
	rm -f ./logfile.log ./supermercato supermercato.PID
