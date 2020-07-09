#!/bin/bash
while [ -e /proc/$2 ]
do
    sleep 1s
done

if [ -f "logfile.log" ]; then
    while read line; do echo $line; done < logfile.log
else
    echo "$0:Errore" 1>&2
fi