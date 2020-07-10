#inizio makefile
#scelgo il compilatore
CC = gcc

#aggiungo eventuali flag standard (-g -pedantic e -Wall)
CFLAGS = -Wall

#aggiungo la libreria usata per i thread
LIBLINK = -lpthread

#file di configurazione standard
CONFIG = ./config/config.txt

#variabile per la modalità di debug
DEBUG = -DDEBUG

#definisco i "falsi" target
.PHONY: clean test personal personalTime debug

all: ./supermercato

./supermercato: ./supermercato.c
	$(CC) $@.c $(CFLAGS) -o $@ $(LIBLINK)

debug: ./supermercato.c
	$(CC) supermercato.c $(CFLAGS) $(DEBUG) -o ./supermercato $(LIBLINK)

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

