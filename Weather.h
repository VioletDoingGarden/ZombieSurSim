#ifndef WEATHER_H
#define WEATHER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <cstdlib>
#include <ctime>

// WeatherSystem class for managing day/night transitions
class WeatherSystem {
public:
    // Weather states
    enum Weather { DAY = 0, NIGHT = 1 };

    // Default constructor
    WeatherSystem();

    // Constructor with renderer and screen dimensions
    WeatherSystem(SDL_Renderer* renderer, int width, int height);

    // Initializes day and night textures
    bool init(SDL_Renderer* renderer, const char* daytimePath, const char* nighttimePath);

    // Checks if system is initialized
    bool isInitialized() const;

    // Sets weather type
    void setType(int type);

    // Gets current weather type
    int getType() const;

    // Updates day/night state based on time
    void update(double currentTime);

    // Renders current weather background
    void render(SDL_Renderer* renderer, int windowWidth, int windowHeight);

    // Frees texture resources
    void cleanup();

private:
    // Daytime background texture
    SDL_Texture* daytimeTex;
    // Nighttime background texture
    SDL_Texture* nighttimeTex;
    // Current background texture
    SDL_Texture* currentBgTex;
    // Start time for weather transitions
    double startTime;
    // Current weather state
    Weather currentWeather;
    // Initialization flag
    bool initialized;
};

#endif

