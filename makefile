constel: graphics.o input.o world.o
	gcc $(CFLAGS) -lm -lGL -lGLEW -lglfw -lfreetype -o constel constel.c graphics.o input.o world.o

graphics.o: graphics.c graphics.h linmath.h
	gcc $(CFLAGS) -I /usr/include/freetype2 -c graphics.c

input.o: input.c input.h
	gcc $(CFLAGS) -c input.c

world.o: world.c world.h linmath.h linmathd.h
	gcc $(CFLAGS) -c world.c

all: constel

clean:
	rm -f *.o *.out constel
