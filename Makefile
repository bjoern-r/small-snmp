CC=gcc
CFLAGS+=-Wall

all: smallsnmp

smallsnmp: main.c snmp.o snmp.h
	$(CC) $(CFLAGS) main.c snmp.o -o smallsnmp
