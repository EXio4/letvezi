
DIR = src/

OBJS = src/main.cpp src/game.cpp

OBJ_NAME = build/game

all : $(OBJS)
	clang++ -std=c++14 $(OBJS) -w -I$(DIR) -lSDL2 -o $(OBJ_NAME)
