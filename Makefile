
SOURCES = src/main.cpp src/game.cpp src/letvetzi.cpp src/persistent.cpp src/binary_serial.cpp
OBJECTS = $(SOURCES:.cpp=.o)
EXECUTABLE = build/letvezi

LDFLAGS = -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer -lpthread

INCLUDES = -I src/ -I libs/

CXXFLAGS = -std=c++14 -O0 -g -Wall -Wextra -Werror $(INCLUDES)

CXX = clang++

all : $(OBJECTS) $(EXECUTABLE)
$(EXECUTABLE): $(OBJECTS)
	mkdir -p build/
	$(CXX) ${LDFLAGS} ${CXXFLAGS} ${OBJECTS} -o $(EXECUTABLE)

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f ${EXECUTABLE} ${OBJECTS}