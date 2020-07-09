#inizio makefile
#scelgo il compilatore
CC = gcc

#aggiungo eventuali flag standard (-g -pedantic e -Wall)
CFLAGS = -Wall

#aggiungo la libreria usata per i thread
LIBLINK = -lpthread

#file di configurazione standard
CONFIG = ./config/config.txt

#definisco i "falsi" target
.PHONY: clean test personal personalTime

all: ./supermercato

./supermercato: ./supermercato.c
	$(CC) $@.c $(CFLAGS) -o $@ $(LIBLINK)

test:
	(./supermercato -c $(CONFIG) & echo $$! > supermercato.txt) &
	sleep 25s; \
	kill -s 1 $$(cat supermercato.txt); \
	chmod +x ./analisi.sh
	./analisi.sh -pid $$(cat supermercato.txt);\

personal:
	(./supermercato -c $(conf) & echo $$! > supermercato.txt) &
	sleep 30s; \
	kill -s 1 $$(cat supermercato.txt); \
	chmod +x ./analisi.sh
	./analisi.sh -pid $$(cat supermercato.txt);\

personalTime:
	(./supermercato -c $(conf) & echo $$! > supermercato.txt) &
	sleep $(time); \
	kill -s 1 $$(cat supermercato.txt); \
	chmod +x ./analisi.sh
	./analisi.sh -pid $$(cat supermercato.txt);\

clean: 
	@echo Pulizia generale...
	rm -f ./logfile.log ./supermercato supermercato.txt

