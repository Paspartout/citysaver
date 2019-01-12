# Todos and Notes

## Goals

- Aim for small size(ideally max 2MB for everything)
- Having fun learning game/graphics programming in C
- Web version using emscripten so that it works everywhere
- Efficient enough for screensaver usage
- Starting point for a topdown/rpg game

### First Version - 13.1.19

- [ ] Project setup
	- [X] Makefile: c and emscripten building
	- [X] SDL2 Boilerplate
- [X] Tilemap rendering
	- [X] Load all tiles and draw them using array
	- [X] Export maps using tiled
- [X] Baisc Sprite rendering
	- [X] Car
- [X] README.md
- [X] Commit to github with pages

### Second Version: Sounds and Simulation

- [ ] Person with walking animation
- [ ] Better Mapping workflow
	- [ ] Learn more tiled
	- [ ] Write script that takes json and exports map.h
- [ ] Better Map
	- [ ] 16:9
	- [ ] Handle layer and sprite keying right
- [ ] Sounds
	- [ ] 8bit street background?
	- [ ] 8bit music?
	- [ ] Simulate car sounds. velocity -> pitch, doppler effect?
- [ ] Simulation?
	- [ ] Simple Road-Traffic simulation
	- [ ] Simulate each persons behavoiur somehow
		- [ ] Probably FSM
		- [ ] Commute: Home -> Work -> Home
		- [ ] Freetime: Home -> Park
		- [ ] Freetime: Work -> Bar
	- [ ] Some special events

### Game/Interactive Version

- [ ] Colision detection
- [ ] Map scrolling
- [ ] Ideas:
	- [ ] Drive cars in traffic simulation
	- [ ] Talk to people to see what they are up to
	- [ ] Enter buildings?

### Misc

- Asserts for bounds checking
- SDL_Log doesnt work in emscipten
- C multidim arrays
- const in C
- Learn webassembly/js profiling
