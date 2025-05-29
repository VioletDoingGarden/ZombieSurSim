#include "Weather.h"
#include <iostream>

// Define weather change interval (in seconds)
   static const double WEATHER_CHANGE_INTERVAL = 30.0; // Change this value to adjust weather switch time

// Default constructor
WeatherSystem::WeatherSystem() 
    : daytimeTex(nullptr), nighttimeTex(nullptr), currentBgTex(nullptr), 
      startTime(0), currentWeather(DAY), initialized(false) {
    srand(static_cast<unsigned int>(time(nullptr)));
}

// Constructor with renderer and dimensions
WeatherSystem::WeatherSystem(SDL_Renderer* renderer, int width, int height)
    : daytimeTex(nullptr), nighttimeTex(nullptr), currentBgTex(nullptr), 
      startTime(0), currentWeather(DAY), initialized(false) {
    srand(static_cast<unsigned int>(time(nullptr)));
    initialized = init(renderer, "day.png", "night.png");
}

// Initializes weather textures
bool WeatherSystem::init(SDL_Renderer* renderer, const char* daytimePath, const char* nighttimePath) {
    daytimeTex = IMG_LoadTexture(renderer, daytimePath);
    nighttimeTex = IMG_LoadTexture(renderer, nighttimePath);
    
    if (!daytimeTex || !nighttimeTex) {
        std::cerr << "Failed to load weather textures: " << IMG_GetError() << "\n";
        cleanup();
        return false;
    }

    currentBgTex = daytimeTex; // Start with daytime
    startTime = SDL_GetTicks() / 1000.0;
    initialized = true;
    return true;
}

// Checks if system is initialized
bool WeatherSystem::isInitialized() const {
    return initialized;
}

// Sets weather type
void WeatherSystem::setType(int type) {
    if (type == 0 || type == 1) {
        currentWeather = static_cast<Weather>(type);
        currentBgTex = (currentWeather == DAY) ? daytimeTex : nighttimeTex;
        startTime = SDL_GetTicks() / 1000.0; // Reset timer
    }
}

// Gets current weather type
int WeatherSystem::getType() const {
    return static_cast<int>(currentWeather);
}

// Updates weather state (day/night) based on WEATHER_CHANGE_INTERVAL
void WeatherSystem::update(double currentTime) {
    double elapsedTime = currentTime - startTime;
    if (elapsedTime >= WEATHER_CHANGE_INTERVAL) { // Modified: Use WEATHER_CHANGE_INTERVAL constant
        currentWeather = (currentWeather == DAY) ? NIGHT : DAY;
        currentBgTex = (currentWeather == DAY) ? daytimeTex : nighttimeTex;
        startTime = currentTime;
    }
}

// Renders current weather background
void WeatherSystem::render(SDL_Renderer* renderer, int windowWidth, int windowHeight) {
    if (currentBgTex) {
        SDL_Rect bgRect = {0, 0, windowWidth, windowHeight};
        SDL_RenderCopy(renderer, currentBgTex, nullptr, &bgRect);
    }
}

// Cleans up weather textures
void WeatherSystem::cleanup() {
    if (daytimeTex) SDL_DestroyTexture(daytimeTex);
    if (nighttimeTex) SDL_DestroyTexture(nighttimeTex);
    daytimeTex = nullptr;
    nighttimeTex = nullptr;
    currentBgTex = nullptr;
    initialized = false;
}

