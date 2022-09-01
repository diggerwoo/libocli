#
# Makefile for libocli
#

SRC = ./src
INC = -I./src
LIB = 

CFLAGS := -O2 -Wall -Wno-unused-but-set-variable -g $(INC)
CC := gcc
AR := ar
RM := rm -rf
CP := cp -rf

default: all

all: libocli.a libocli.so

OBJS =	$(SRC)/lex.o $(SRC)/ocli_core.o $(SRC)/ocli_rl.o	\
	$(SRC)/symbol.o $(SRC)/utils.o $(SRC)/cmd_built_in.o

HDRS =	$(SRC)/lex.h $(SRC)/ocli.h $(SRC)/lex.h

libocli.a: $(OBJS) $(HDRS)
	$(AR) r $@ $^

lexdebug: $(SRC)/lex.c $(SRC)/lex.h
	$(CC) $(CFLAGS) -DDEBUG_LEX_MAIN -o lexdebug $(SRC)/lex.c -lpcre
	
DEMODIR = ./example
DEMOSRC = $(DEMODIR)/democli.c $(DEMODIR)/netutils.c $(DEMODIR)/show.c	\
	  $(DEMODIR)/route.c

demo: $(DEMOSRC) libocli.a
	$(CC) $(CFLAGS) -o democli $(DEMOSRC) -locli -lpcre -lreadline

libocli.so: $(OBJS) $(HDRS)
	rm -rf $(OBJS)
	$(CC) $(CFLAGS) -fpic -o $(SRC)/lex.o -c $(SRC)/lex.c
	$(CC) $(CFLAGS) -fpic -o $(SRC)/ocli_core.o -c $(SRC)/ocli_core.c
	$(CC) $(CFLAGS) -fpic -o $(SRC)/ocli_rl.o -c $(SRC)/ocli_rl.c
	$(CC) $(CFLAGS) -fpic -o $(SRC)/symbol.o -c $(SRC)/symbol.c
	$(CC) $(CFLAGS) -fpic -o $(SRC)/utils.o -c $(SRC)/utils.c
	$(CC) $(CFLAGS) -fpic -o $(SRC)/cmd_built_in.o -c $(SRC)/cmd_built_in.c
	$(CC) $(CFLAGS) -shared -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

install: libocli.so $(HDRS)
	install -m 755 -o root -g root libocli.so /usr/local/lib/
	install -m 755 -o root -g root libocli.a /usr/local/lib/
	install -m 644 -o root -g root -D $(SRC)/list.h /usr/local/include/ocli/list.h
	install -m 644 -o root -g root -D $(SRC)/ocli_defs.h /usr/local/include/ocli/ocli_defs.h
	install -m 644 -o root -g root -D $(SRC)/lex.h /usr/local/include/ocli/lex.h
	install -m 644 -o root -g root -D $(SRC)/ocli.h /usr/local/include/ocli/ocli.h

clean:
	-$(RM) libocli.a libocli.so lexdebug democli $(SRC)/*.o
