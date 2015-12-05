
DIR = -I src/ -I libs/

OBJS = src/main.cpp src/game.cpp src/letvetzi.cpp src/persistent.cpp src/binary_serial.cpp

OBJ_NAME = build/game

LIBS = -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lpthread

WARNS = -Wall -Wextra -Werror "-Wno-\#warnings"

CXX = clang++

all : $(OBJS)
	mkdir -p build/
	$(CXX) -std=c++14 $(OBJS) -g -O0 $(WARNS) $(DIR) $(LIBS) -o $(OBJ_NAME)
