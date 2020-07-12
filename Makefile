#inizio makefile
#scelgo il compilatore
CC = gcc

#aggiungo eventuali flag standard (-g -pedantic e -Wall)
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200112L -Wall

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
	(./supermercato -c $(CONFIG) & echo $$! > supermercato.temp) &
	sleep 25s; \
	kill -s 1 $$(cat supermercato.temp); \
	chmod +x ./analisi.sh
	./analisi.sh -file logfile.log -pid $$(cat supermercato.temp);\

personal:
	(./supermercato -c $(conf) & echo $$! > supermercato.temp) &
	sleep 30s; \
	kill -s 1 $$(cat supermercato.temp); \
	chmod +x ./analisi.sh
	./analisi.sh -file logfile.log -pid $$(cat supermercato.temp);\

personalTime:
	(./supermercato -c $(conf) & echo $$! > supermercato.temp) &
	sleep $(time); \
	kill -s 1 $$(cat supermercato.temp); \
	chmod +x ./analisi.sh
	./analisi.sh -file logfile.log -pid $$(cat supermercato.temp);\

clean: 
	@echo Pulizia generale...
	rm -f ./logfile.log ./casse.log ./supermercato supermercato.temp

