# Makefile for Zombie Shooter Game

CC = g++
CFLAGS = -Wall -std=c++11
LIBS = -lGL -lGLU -lglut -lSDL2 -lSDL2_mixer -lm

TARGET = zombie_shooter
SRC = main.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

# Install dependencies (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install -y libgl1-mesa-dev freeglut3-dev libsdl2-dev libsdl2-mixer-dev

.PHONY: all clean run install-deps
