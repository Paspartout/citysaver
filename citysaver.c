#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <SDL2/SDL.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

// Config
// ============================================================================
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

#define TITLE "Citysaver"
// ============================================================================

static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *canvas = NULL;
static bool running = true;
static SDL_Rect camera = {.x = 0, .y = 0, .w = 320, .h = 240};

static SDL_Texture *tilesheet_texture = NULL;
static SDL_Texture *drawn_background = NULL;

#include "map.h"

#define TILE_SIZE 16
#define MAP_WIDTH 32 * TILE_SIZE
#define MAP_HEIGHT 32 * TILE_SIZE

// ============================================================================
// Background
// ============================================================================

void predraw_background();

int setup_background() {
	// Load atlas from bmp
	// TODO: Probably load from png to save space later
	SDL_Surface *bgtiles_surface = SDL_LoadBMP("gfx/tilemap_packed.bmp");
	if (bgtiles_surface == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "error loading background: %s\n", SDL_GetError());
		return -1;
	}
	SDL_SetColorKey(bgtiles_surface, SDL_TRUE, SDL_MapRGB(bgtiles_surface->format, 255, 0, 255));
	tilesheet_texture = SDL_CreateTextureFromSurface(renderer, bgtiles_surface);
	if (tilesheet_texture == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "error creating background texture: %s\n",
		             SDL_GetError());
		SDL_FreeSurface(bgtiles_surface);
		return -1;
	}
	SDL_FreeSurface(bgtiles_surface);
	predraw_background();

	return 0;
}

void free_background() {
	SDL_DestroyTexture(drawn_background);
	SDL_DestroyTexture(tilesheet_texture);
}

/// Background drawing
void draw_tilemap(int16_t map[32][32]) {
	SDL_Rect dest = {.x = 0, .y = 0, .w = 16, .h = 16};
	SDL_Rect src = {.x = 0, .y = 0, .w = 16, .h = 16};
	for (int x = 0; x < 32; x++) {
		for (int y = 0; y < 32; y++) {
			dest.x = x * 16;
			dest.y = y * 16;
			// TODO: is this inefficient?
			const int16_t tile = map[y][x];
			src.x = (tile % 27) * 16;
			src.y = (tile / 27) * 16;
			if (tile >= 0) {
				SDL_RenderCopy(renderer, tilesheet_texture, &src, &dest);
			}
		}
	}
}

void predraw_background() {
	drawn_background = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
	                                     SDL_TEXTUREACCESS_TARGET, MAP_HEIGHT, MAP_HEIGHT);
	SDL_SetRenderTarget(renderer, drawn_background);
	draw_tilemap(street);
	draw_tilemap(buildings);
	draw_tilemap(buildings_top);
	SDL_SetRenderTarget(renderer, NULL);
}

/* static void print_rect(const SDL_Rect rect) { */
/* 	printf("x: %d, y: %d, w: %d, h: %d\n", rect.x, rect.y, rect.w, rect.h);
 */
/* } */

void draw_background() {
	SDL_Rect dest = {.x = -camera.x, .y = -camera.y, .w = MAP_WIDTH, .h = MAP_HEIGHT};
	SDL_Rect src = {.x = 0, .y = 0, .w = MAP_WIDTH, .h = MAP_HEIGHT};

	SDL_RenderCopy(renderer, drawn_background, &src, &dest);
}

// ============================================================================
// Cars
// ============================================================================

enum Facing {
	Right,
	Down,
	Left,
	Up,
};

typedef struct Car {
	int m_x;   // x = milli_x * 1024
	int m_y;   // y = milli_x * 1024
	int m_vel; // velocity is 1024

	int x; // x = milli_x / 1024
	int y; // y = milli_x / 1024

	enum Facing facing;
} Car;

// All offsets and sizes of the red car
// TODO: map all other sprites
SDL_Rect tilemap_redcar[4] = {
    {.x = 245, .y = 266, .w = 22, .h = 22}, // Right
    {.x = 272, .y = 267, .w = 16, .h = 21}, // Down
    {.x = 293, .y = 266, .w = 22, .h = 22}, // Left
    {.x = 320, .y = 267, .w = 16, .h = 31}, // Up
};

#define NUM_CARS 4
Car cars[NUM_CARS];

void draw_car(const Car *car) {
	const SDL_Rect *car_rect = &tilemap_redcar[car->facing];
	SDL_Rect dest = {.x = car->x, .y = car->y, .w = car_rect->w, car_rect->h};

	SDL_RenderCopy(renderer, tilesheet_texture, car_rect, &dest);
}

void draw_cars() {
	for (int i = 0; i < NUM_CARS; i++) {
		draw_car(&cars[i]);
	}
}

void init_car(Car *car, enum Facing facing, int x, int y, int vel) {
	car->facing = facing;
	car->x = x;
	car->y = y;
	car->m_x = x * 1024;
	car->m_y = y * 1024;
	car->m_vel = vel;
}

#define MIN_SPEED 512
#define MAX_SPEED 2048
#define RANDOM_SPEED() ((random() % (MAX_SPEED - MIN_SPEED)) + MIN_SPEED)

void init_cars() {
	// TODO: Use better rng?
	int speed = RANDOM_SPEED();
	init_car(&cars[0], Left, MAP_WIDTH + tilemap_redcar[Left].w,
	         (40 - (tilemap_redcar[Left].h / 2)), speed);
	speed = RANDOM_SPEED();
	init_car(&cars[1], Right, (-tilemap_redcar[Right].w), (64 - (tilemap_redcar[Right].h / 2)),
	         speed);
	speed = RANDOM_SPEED();
	init_car(&cars[2], Left, MAP_WIDTH + tilemap_redcar[Left].w,
	         (268 - (tilemap_redcar[Left].h / 2)), speed);
	speed = RANDOM_SPEED();
	init_car(&cars[3], Right, (-tilemap_redcar[Right].w), (290 - (tilemap_redcar[Right].h / 2)),
	         speed);
}

void animate_car(Car *car) {
	// TODO: Tweening? chugging animation?
	switch (car->facing) {
	case Right:
		car->m_x += car->m_vel;
		car->x = car->m_x / 1024; // Should be a shift
		break;
	case Down:
		car->m_y += car->m_vel;
		car->y = car->m_y / 1024; // Should be a shift
		break;
	case Left:
		car->m_x -= car->m_vel;
		car->x = car->m_x / 1024; // Should be a shift
		/* printf("mx: %d x: %d\n", car->m_x, car->x); */
		break;
	case Up:
		car->m_y += car->m_vel;
		car->y = car->m_y / 1024; // Should be a shift
		break;
	}

	const int car_width = tilemap_redcar[car->facing].w;
	if (car->x > MAP_WIDTH + car_width * 2) {
		init_car(car, car->facing, -car_width, car->y, RANDOM_SPEED());
	} else if (car->x < -car_width * 2) {
		init_car(car, car->facing, MAP_WIDTH + car_width, car->y, RANDOM_SPEED());
	}
}

void animate_cars() {
	for (int i = 0; i < NUM_CARS; i++) {
		animate_car(&cars[i]);
	}
}

// ============================================================================
// Setup and Mainloop
// ============================================================================

/// Initialize SDL and everything needed
int init() {
	if (SDL_Init(SDL_INIT_VIDEO) < 0) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "error initializing SDL: %s\n", SDL_GetError());
		return -1;
	}

	window = SDL_CreateWindow(TITLE, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH,
	                          SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (window == NULL) {
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "error creating SDL window: %s\n", SDL_GetError());
		SDL_Quit();
		return -1;
	}

	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (renderer == NULL) {
		SDL_DestroyWindow(window);
		SDL_Quit();
		SDL_LogCritical(SDL_LOG_CATEGORY_ERROR, "error creating SDL renderer: %s\n",
		                SDL_GetError());
		return -1;
	}

	SDL_RenderSetLogicalSize(renderer, 512, 512);

	return 0;
}

/// Deinitialize SDL
void deinit() {
	SDL_DestroyTexture(canvas);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

/// Main loop iteration
void loop_tick() {
	// Handle inputs
	SDL_Event e;
	while (SDL_PollEvent(&e)) {
		if (e.type == SDL_QUIT) {
			running = false;
			break;
		}
	}

	// Logic
	animate_cars();

	// Draw screen
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	draw_background();
	draw_cars();

	SDL_RenderPresent(renderer);

	// TODO: Proper fps limit and calculation
	SDL_Delay(15);
}

int main() {
	if (init() != 0) {
		return -1;
	}
	if (setup_background() != 0) {
		fprintf(stderr, "error setting up background\n");
		deinit();
		return -1;
	}
	predraw_background();
	srandom(time(NULL));
	init_cars();

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(loop_tick, 0, 1);
#else
	while (running) {
		loop_tick();
	}
#endif

	free_background();
	deinit();
	return 0;
}
