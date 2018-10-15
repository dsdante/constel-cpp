constel: common.o graphics.o input.o world.o
	gcc $(CFLAGS) -lm -lGL -lGLEW -lglfw -lfreetype -o constel constel.c common.o graphics.o input.o world.o

common.o: common.c common.h
	gcc -std=c11 -fms-extensions $(CFLAGS) -c common.c

graphics.o: graphics.c graphics.h
	gcc -std=c11 -fms-extensions $(CFLAGS) -I /usr/include/freetype2 -c graphics.c

input.o: input.c input.h
	gcc -std=c11 -fms-extensions $(CFLAGS) -c input.c

world.o: world.c world.h
	gcc -std=c11 -fms-extensions $(CFLAGS) -c world.c

all: constel

clean:
	rm -f *.o *.out constel
