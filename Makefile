CFLAGS := `sdl2-config --cflags` -Og -g -Wall -Wextra
EMFLAGS := -O2 -s USE_SDL=2
LDLIBS := -lSDL2

# TODO: Add more deps like gfx, sounds and headers


run: citysaver
	./$^

all: citysaver citysaver.html

citysaver.html: citysaver.c
	emcc --embed-file gfx/tilemap_packed.bmp $^ ${EMFLAGS} -o $@

citysaver: citysaver.o
citysaver.o: animation.h

clean:
	rm -f citysaver *.o citysaver.html citysaver.js citysaver.wasm

.PHONY: all run clean
