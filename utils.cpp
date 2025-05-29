#include "utils.h"
#include <iostream>
// - renderer: The SDL_Renderer used to create the texture
// Returns:
// - A pointer to the SDL_Texture object if successful, or nullptr if loading fails

SDL_Texture* loadTexture(const std::string& path, SDL_Renderer* renderer) {
    // Load the image into an SDL_Surface object using SDL_image's IMG_Load function
    SDL_Surface* surf = IMG_Load(path.c_str());
    if (!surf) { // Check if the surface was loaded successfully
        std::cerr << "Failed to load image: " << path << "\n"; // Log an error message if loading fails
        return nullptr; // Return nullptr to indicate failure
    }
    // Create an SDL_Texture from the loaded surface
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    // Free the surface memory as it is no longer needed
    SDL_FreeSurface(surf);
    // Return the created texture
    return tex;
}