CLIBS = 
CFLAGS =
YFLAGS = -d
YACC = yacc
CC = cc

OBJECTS = parse_rules.o
TARGET = muv

muv: ${OBJECTS}
	${CC} ${CFLAGS} -o $@ ${OBJECTS} ${CLIBS}

parse_rules.o: y.tab.c
	${CC} ${CFLAGS} -c y.tab.c -o parse_rules.o

y.tab.c: parse_rules.y
	${YACC} -d parse_rules.y

clean:
	rm -f core y.tab.c parse_rules.o y.tab.h
