
.PHONY: Hydroponics
Hydroponics:
	gcc -o Hydroponics Hydroponics.c -lmysqlclient -lwiringPi -std=gnu99
