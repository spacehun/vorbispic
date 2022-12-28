CC = gcc
CF = -s -O2 -Wall

vorbispic: main.o base64.o
	$(CC) $(CF) -o vorbispic main.o base64.o

main.o: main.c
	$(CC) $(CF) -c -o main.o main.c

base64.o: base64.c
	$(CC) $(CF) -c -o base64.o base64.c

clean:
	rm -f vorbispic main.o base64.o

install: vorbispic
	cp vorbispic /usr/local/bin

all: vorbispic
	
