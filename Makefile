CC=gcc
CFLAGS+=-Wall -g

all: smallsnmp

smallsnmp: main.c snmp.o snmp.h db.o db.h
	$(CC) $(CFLAGS) main.c snmp.o db.o -o smallsnmp

.PHONY: clean
clean:
	@rm -f *.o
	@rm -f sml_server

