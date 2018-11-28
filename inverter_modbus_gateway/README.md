Modbus-Gateway
==============


Per eseguire il gateway: cd ~/modbus-gateway; ./modbus-gateway.py

(ci preoccuperemo poi di predisporre l'avvio automatico al boot)

Se il gateway non viene chiuso con il ctrl-c, usare il seguente comando:
    killall -9 python3

Per testare il server, usando un terzo convertitore sul rasp, usare l'utility mbpoll:

    mbpoll  -b 9600 -P none  /dev/ttyUSB2 -t4  -c10


