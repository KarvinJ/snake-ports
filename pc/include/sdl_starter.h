#pragma once

#include <SDL2/SDL.h>

const int CELL_SIZE = 20;
const int CELL_COUNT = 27;

const int SCREEN_WIDTH = CELL_SIZE * CELL_COUNT;
const int SCREEN_HEIGHT = CELL_SIZE * CELL_COUNT;
const int FRAME_RATE = 60;

int startSDL(SDL_Window *window, SDL_Renderer *renderer);

void capFrameRate(Uint32 frameStartTime);