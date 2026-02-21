
CC = gcc 

EXENAME = tanks

all:
	$(CC) main.c -o $(EXENAME) -lglut -lGL -lGLEW
