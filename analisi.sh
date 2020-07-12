#!/bin/bash
while [ -e /proc/$4 ]
do
    sleep 1s
done

if [ -f $2 ]; then
    while read line; do echo $line; done < $2
    if [ -f "casse.log" ]; then
        cat casse.log
    else
        echo "$0:Errore" 1>&2
    fi
else
    echo "$0:Errore" 1>&2
fi