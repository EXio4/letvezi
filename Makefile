
SOURCES = src/main.cpp src/game.cpp src/render.cpp src/letvetzi.cpp src/persistent.cpp src/binary_serial.cpp
HEADERS = src/game.h src/render.h src/letvetzi.h src/persistent.h src/binary_serial.h
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

src/main.o: src/main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

src/game.o: src/game.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

src/render.o: src/render.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

src/letvetzi.o: src/letvetzi.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

src/persistent.o: src/persistent.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

src/binary_serial.o: src/binary_serial.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f ${EXECUTABLE} ${OBJECTS}