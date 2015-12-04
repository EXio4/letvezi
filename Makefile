
DIR = src/

OBJS = src/main.cpp src/game.cpp src/persistent.cpp src/binary_serial.cpp

OBJ_NAME = build/game

LIBS = -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lpthread

WARNS = -Wall -Wextra -Werror -pedantic

CXX = clang++

all : $(OBJS)
	mkdir -p build/
	$(CXX) -std=c++14 $(OBJS) -g -O0 $(WARNS) -I$(DIR) $(LIBS) -o $(OBJ_NAME)
