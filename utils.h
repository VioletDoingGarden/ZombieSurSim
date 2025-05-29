#ifndef UTILS_H
#define UTILS_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <string>

// Function to load texture
SDL_Texture* loadTexture(const std::string& path, SDL_Renderer* renderer);

#endif // UTILS_H