constel-cpp: common.o graphics.o input.o world.o
	gcc $(CXXFLAGS) -lm -lpthread -lGL -lGLEW -lglfw -lfreetype -o constel-cpp constel.cpp common.o graphics.o input.o world.o

common.o: common.cpp common.hpp 
	gcc -std=c++17 $(CXXFLAGS) -c common.cpp

graphics.o: graphics.cpp graphics.hpp 
	gcc -std=c++17 $(CXXFLAGS) -I /usr/include/freetype2 -c graphics.cpp

input.o: input.cpp input.hpp 
	gcc -std=c++17 $(CXXFLAGS) -c input.cpp

world.o: world.cpp world.hpp 
	gcc -std=c++17 -fms-extensions $(CXXFLAGS) -c world.cpp

all: constel

clean:
	rm -f *.o *.out constel-cpp
