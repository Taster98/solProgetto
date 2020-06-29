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


