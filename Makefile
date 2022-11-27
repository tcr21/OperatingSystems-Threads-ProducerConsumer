CC=g++ -Wall

all: main

main: helper.o main.o
	$(CC) -pthread -g -o main helper.o main.o

main.o: helper.cc main.cc
	$(CC) -c -g helper.cc main.cc

tidy:
	rm -f *.o core

clean:
	rm -f main producer consumer *.o core
