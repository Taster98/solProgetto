/* controlla -1; stampa errore e termina */
#define ec_meno1(s,m) \
    if((s)==-1){ \
        perror(m); \
        exit(EXIT_FAILURE); \
    }

/* controlla 0; stampa errore e termina */
#define ec_zero(s,m) \
    if((s)!=0){ \
        perror(m); \
        exit(EXIT_FAILURE); \
    }

/* controlla NULL; stampa errore (NULL) */
#define ec_null(s,m) \
    if((s) == NULL){ \
        fprintf(stderr, m "\n"); \
    }
/*controlla la mutex_unlock, restituisce un numero diverso da 0 se non è andata a buon fine*/
#define ec_mutex_unlock(s) \
    if((s)!=0)  \
        fprintf(stderr, "Errore con la unlock");
/*controlla la mutex_lock, restituisce un numero diverso da 0 se non è andata a buon fine*/
#define ec_mutex_lock(s) \
    if((s)!=0)  \
        fprintf(stderr, "Errore con la lock");


