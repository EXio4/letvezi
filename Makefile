
DIR = src/

OBJS = src/main.cpp src/game.cpp

OBJ_NAME = build/game

LIBS = -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lpthread

all : $(OBJS)
	clang++ -std=c++14 $(OBJS) -w -I$(DIR) $(LIBS) -o $(OBJ_NAME)
