## nes emulator written in C with SDL

### progress 

Donkey Kong and Pac-Man are both functional. Only games that use the default mapper are currently supported.

* [x] CPU
* PPU
    * [x] sprites
    * [x] background
    * [x] correct colors
    * [ ] 8x16 sprites
    * [ ] scrolling
* [ ] mappers
* [ ] APU
* [ ] configurable controls

### building 

dependencies: gcc, SDL2, and pkg-config (for cross platform SDL flags)

build steps
* run `make` from the root of the repo
* to run: `./cnes <path/to/rom>`

### button mapping

this is the default button mapping, which is currently not configurable

* up, down, left, right => w, s, a, d
* a => j
* b => k 
* select => space
* start => enter 
* escape to exit

