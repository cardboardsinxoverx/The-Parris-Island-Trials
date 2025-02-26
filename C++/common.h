#ifndef COMMON_H
#define COMMON_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_mixer.h>
#include <ctime> // For time functions

const int WIDTH = 800;
const int HEIGHT = 600;
const int MAP_WIDTH = 1600;  // Adjust based on map scale
const int MAP_HEIGHT = 1200; // Adjust based on map scale
const int SPRITE_SIZE = 32;

// Colors (SDL uses RGB values 0-255)
const SDL_Color SAND = {194, 178, 128, 255};    // Parris Island ground (day)
const SDL_Color NIGHT = {80, 80, 100, 255};     // Night color (bluish-gray, distinct from roads)
const SDL_Color GREEN = {0, 255, 0, 255};       // Recruit
const SDL_Color TAN = {210, 180, 140, 255};     // Drill Instructor
const SDL_Color BLACK = {0, 0, 0, 255};         // Background for UI
const SDL_Color WHITE = {255, 255, 255, 255};   // For text/UI
const SDL_Color BROWN = {139, 69, 19, 255};     // Obstacles/Particles/Brick Buildings
const SDL_Color GRAY = {128, 128, 128, 255};    // Roads/Buildings
const SDL_Color DARK_SAND = {150, 130, 100, 255};  // Sand pits
const SDL_Color GRASS = {0, 100, 0, 255};       // Rifle range
const SDL_Color PAVEMENT = {255, 255, 255, 255}; // Parade deck
const SDL_Color WATER = {0, 119, 190, 255};     // Teal for water bodies

struct Vector2 {
    float x, y;
    Vector2(float x_ = 0, float y_ = 0) : x(x_), y(y_) {}
};

#endif // COMMON_H
