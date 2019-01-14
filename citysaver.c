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

// TODO: Make map.h from json using a utility program
#include "map.h"

#define TILE_SIZE 16
#define MAP_WIDTH 32 * TILE_SIZE
#define MAP_HEIGHT 32 * TILE_SIZE

// ============================================================================
// Graphics
// ============================================================================

static SDL_Texture *tilesheet_texture = NULL;

int background_init();
void background_deinit();

int graphics_init() {
	// Load atlas from bmp
	// TODO: Probably load from png to save space later
	SDL_Surface *tilesheet = SDL_LoadBMP("gfx/tilemap_packed.bmp");
	if (tilesheet == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "error loading background: %s\n", SDL_GetError());
		return -1;
	}
	SDL_SetColorKey(tilesheet, SDL_TRUE, SDL_MapRGB(tilesheet->format, 255, 0, 255));

	// Load sheet to texture
	tilesheet_texture = SDL_CreateTextureFromSurface(renderer, tilesheet);
	if (tilesheet_texture == NULL) {
		SDL_LogError(SDL_LOG_CATEGORY_ERROR, "error creating background texture: %s\n",
		             SDL_GetError());
		SDL_FreeSurface(tilesheet);
		return -1;
	}
	SDL_FreeSurface(tilesheet);

	background_init();
	return 0;
}

void graphics_deinit() {
	background_deinit();
	SDL_DestroyTexture(tilesheet_texture);
}

// ============================================================================
// Background
// ============================================================================

// TODO: Draw seperate layers for better visauls/occlusion
static SDL_Texture *drawn_background = NULL;

void background_predraw();
int background_init() {
	background_predraw();
	return 0;
}

void background_deinit() { SDL_DestroyTexture(drawn_background); }

/// Background drawing
void background_draw_tilemap(int16_t map[32][32]) {
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

void background_predraw() {
	drawn_background = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
	                                     SDL_TEXTUREACCESS_TARGET, MAP_HEIGHT, MAP_HEIGHT);
	SDL_SetRenderTarget(renderer, drawn_background);
	background_draw_tilemap(street);
	background_draw_tilemap(buildings);
	background_draw_tilemap(buildings_top);
	SDL_SetRenderTarget(renderer, NULL);
}

/* static void print_rect(const SDL_Rect rect) { */
/* 	printf("x: %d, y: %d, w: %d, h: %d\n", rect.x, rect.y, rect.w, rect.h);
 */
/* } */

void background_draw() {
	SDL_Rect dest = {.x = -camera.x, .y = -camera.y, .w = MAP_WIDTH, .h = MAP_HEIGHT};
	SDL_Rect src = {.x = 0, .y = 0, .w = MAP_WIDTH, .h = MAP_HEIGHT};

	SDL_RenderCopy(renderer, drawn_background, &src, &dest);
}

// ============================================================================
// Cars
// ============================================================================

enum Direction {
	Right,
	Down,
	Left,
	Up,
};

typedef struct Car {
	int x;        // x = milli_x * 1024
	int y;        // y = milli_x * 1024
	int velocity; // velocity is in 1/1024pix/tick

	int x_px; // x = milli_x / 1024
	int y_px; // y = milli_x / 1024

	enum Direction direction;
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

void car_draw(const Car *car) {
	const SDL_Rect *car_rect = &tilemap_redcar[car->direction];
	SDL_Rect dest = {.x = car->x_px, .y = car->y_px, .w = car_rect->w, car_rect->h};

	SDL_RenderCopy(renderer, tilesheet_texture, car_rect, &dest);
}

void car_setpos(Car *car, int x, int y) {
	car->x = x;
	car->y = y;
	car->x_px = x / 1024;
	car->y_px = y / 1024;
}

void car_setpos_px(Car *car, int x_px, int y_px) {
	car->x_px = x_px;
	car->y_px = y_px;
	car->x = x_px * 1024;
	car->y = y_px * 1024;
}

void car_init(Car *car, enum Direction direction, int x, int y, int vel) {
	car->direction = direction;
	car->velocity = vel;
	car_setpos(car, x, y);
}

void car_init_px(Car *car, enum Direction direction, int x, int y, int vel) {
	car->direction = direction;
	car->velocity = vel;
	car_setpos_px(car, x, y);
}

#define MIN_SPEED 512
#define MAX_SPEED 2048
#define RANDOM_SPEED() ((random() % (MAX_SPEED - MIN_SPEED)) + MIN_SPEED)

void car_run_physics(Car *car) {
	switch (car->direction) {
	case Right:
		car->x += car->velocity;
		car->x_px = car->x / 1024; // Should be a shift
		break;
	case Down:
		car->y += car->velocity;
		car->y_px = car->y / 1024; // Should be a shift
		break;
	case Left:
		car->x -= car->velocity;
		car->x_px = car->x / 1024; // Should be a shift
		break;
	case Up:
		car->y += car->velocity;
		car->y_px = car->y / 1024; // Should be a shift
		break;
	}

	printf("x: %d, y: %d\n", car->x_px, car->y_px);

	// Car respawning
	const int car_width = tilemap_redcar[car->direction].w;
	if (car->x_px > MAP_WIDTH + car_width * 2) {
		car_init_px(car, car->direction, -car_width, car->y_px, RANDOM_SPEED());
	} else if (car->x_px < -car_width * 2) {
		car_init_px(car, car->direction, MAP_WIDTH + car_width, car->y_px, RANDOM_SPEED());
	}
}

void cars_init() {
	// TODO: Use better rng?
	int speed = RANDOM_SPEED();
	car_init_px(&cars[0], Left, MAP_WIDTH + tilemap_redcar[Left].w,
	            (40 - (tilemap_redcar[Left].h / 2)), speed);
	speed = RANDOM_SPEED();
	car_init_px(&cars[1], Right, (-tilemap_redcar[Right].w), (64 - (tilemap_redcar[Right].h / 2)),
	            speed);
	speed = RANDOM_SPEED();
	car_init_px(&cars[2], Left, MAP_WIDTH + tilemap_redcar[Left].w,
	            (268 - (tilemap_redcar[Left].h / 2)), speed);
	speed = RANDOM_SPEED();
	car_init_px(&cars[3], Right, (-tilemap_redcar[Right].w), (290 - (tilemap_redcar[Right].h / 2)),
	            speed);
}

void cars_draw() {
	for (int i = 0; i < NUM_CARS; i++) {
		car_draw(&cars[i]);
	}
}

void cars_run_physics() {
	for (int i = 0; i < NUM_CARS; i++) {
		car_run_physics(&cars[i]);
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
	cars_run_physics();

	// Draw screen
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	background_draw();
	cars_draw();

	SDL_RenderPresent(renderer);

	// TODO: Proper fps limit and calculation
	SDL_Delay(15);
}

int main() {
	if (init() != 0) {
		return -1;
	}
	if (graphics_init() != 0) {
		fprintf(stderr, "error setting up background\n");
		deinit();
		return -1;
	}
	srandom(time(NULL));
	cars_init();

#ifdef __EMSCRIPTEN__
	emscripten_set_main_loop(loop_tick, 0, 1);
#else
	while (running) {
		loop_tick();
	}
#endif

	graphics_deinit();
	background_deinit();
	deinit();
	return 0;
}
