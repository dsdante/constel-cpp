constel: common.o graphics.o input.o world.o
	gcc $(CFLAGS) -lm -lGL -lGLEW -lglfw -lfreetype -o constel constel.c common.o graphics.o input.o world.o

common.o: common.c common.h
	gcc $(CFLAGS) -c common.c

graphics.o: graphics.c common.h graphics.h linmath.h
	gcc $(CFLAGS) -I /usr/include/freetype2 -c graphics.c

input.o: input.c input.h
	gcc $(CFLAGS) -c input.c

world.o: world.c world.h common.h linmath.h linmathd.h
	gcc $(CFLAGS) -c world.c

all: constel

clean:
	rm -f *.o *.out constel
