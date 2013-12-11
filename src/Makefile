CC = gcc
CFLAGS = -g -c
LD = gcc
LDFLAGS = -g -lreadline


rxh: rxh.o rxCommand.o wrappers.o rxLib.o rxFunctions.o rxJobs.o
	${LD} ${LDFLAGS} rxh.o rxCommand.o wrappers.o rxLib.o rxFunctions.o rxJobs.o -o rxh

test2: test2.o rxCommand.o wrappers.o
	${LD} ${LDFLAGS} test2.o rxCommand.o wrappers.o -o test2
	
test2.o: test2.c rxCommand.h wrappers.h
	${CC} ${CFLAGS} test2.c

rxh.o: rxh.c rxCommand.h wrappers.h
	${CC} ${CFLAGS} rxh.c

rxJobs.o: rxJobs.h rxJobs.c
	${CC} ${CFLAGS} rxJobs.c

rxFunctions.o: rxFunctions.h rxFunctions.c
	${CC} ${CFLAGS} rxFunctions.c

rxLib.o: rxLib.h rxLib.c
	${CC} ${CFLAGS} rxLib.c

rxCommand.o: rxCommand.h rxCommand.c
	${CC} ${CFLAGS} rxCommand.c

#rxLList.o: rxLList.h rxLList.c wrappers.h
#	${CC} ${CFLAGS} rxLList.c

wrappers.o: wrappers.c wrappers.h
	${CC} ${CFLAGS} wrappers.c
clean:
	rm -rf rxh test2 *.o
