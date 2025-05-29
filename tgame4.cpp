#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <random>
#include <algorithm>
#include "utils.h"
#include "Weather.h"

// Screen and game constants
const int SCREEN_WIDTH = 800; // Width of the game window
const int SCREEN_HEIGHT = 600; // Height of the game window
const int TILE_SIZE = 32; // Size of each tile in pixels
const int MAX_NAME_LENGTH = 10; // Maximum length of player name

// Physics constants for movement and interactions
const float GRAVITY = 0.5f; // Gravity force applied to entities
const float JUMP_FORCE = -13.0f; // Upward force for jumping
const float PLAYER_ACCEL = 0.8f; // Player acceleration rate
const float FRICTION = 0.7f; // Friction to slow down movement
const float MAX_SPEED = 5.0f; // Maximum horizontal speed for entities

// Vector2D structure for 2D position and velocity
struct Vector2D {
    float x, y; // X and Y coordinates
    Vector2D(float x = 0, float y = 0) : x(x), y(y) {} // Constructor with default values
    void add(const Vector2D& v) { x += v.x; y += v.y; } // Add another vector
    void mul(float scalar) { x *= scalar; y *= scalar; } // Multiply vector by a scalar
    void set(float x_, float y_) { x = x_; y = y_; } // Set vector components
};

// Platform structure to represent static platforms
struct Platform {
    int x, y, width, height; // Position and dimensions in tile units
    SDL_Texture* texture; // Texture for rendering the platform
};

// Terrain class to manage collision detection with platforms
class Terrain {
public:
    std::vector<Platform> platforms; // List of platforms
    Terrain(const std::vector<Platform>& plats) : platforms(plats) {} // Constructor initializing platforms

    // Check if a point is solid (within a platform)
    bool getSolid(int x, int y) {
        for (const auto& platform : platforms) {
            SDL_Rect platformRect = {platform.x * TILE_SIZE, platform.y * TILE_SIZE, platform.width * TILE_SIZE, platform.height * TILE_SIZE};
            if (x >= platformRect.x && x < platformRect.x + platformRect.w &&
                y >= platformRect.y && y < platformRect.y + platformRect.h) {
                return true; // Point is within a platform
            }
        }
        return false; // Point is not within any platform
    }
};

// Base class for entities with physics (player, zombies, food)
class PhysicsEntity {
public:
    Vector2D pos, vel, col; // Position, velocity, and collision box offset
    int w, h; // Width and height of the entity
    SDL_Texture* runTextures[10]; // Animation textures for running (10 frames)
    SDL_Texture* standTextures[12]; // Animation textures for standing (12 frames)
    int curFrame; // Current animation frame
    float frameTime; // Time since last frame update
    float frameSpeed; // Speed of animation frame changes
    bool onGround; // Whether the entity is on the ground
    bool gravity; // Whether gravity is applied
    bool friction; // Whether friction is applied
    int health; // Entity health
    int lastDirection; // Last movement direction for rendering

    // Constructor initializing position, size, and textures
    PhysicsEntity(float x, float y, int w_, int h_, SDL_Texture* runTex[], SDL_Texture* standTex[])
        : pos(x, y), vel(0, 0), w(w_), h(h_), curFrame(0), frameTime(0.0f), frameSpeed(0.08f),
          onGround(false), gravity(true), friction(true), health(100), lastDirection(1) {
        col.set(w_, h_); // Set collision box to match size
        for (int i = 0; i < 10; i++) {
            runTextures[i] = runTex[i]; // Copy run animation textures
        }
        for (int i = 0; i < 12; i++) {
            standTextures[i] = standTex[i]; // Copy stand animation textures
        }
    }

    // Virtual destructor for proper cleanup in derived classes
    virtual ~PhysicsEntity() {}

    // Render the entity based on movement state
    virtual void render(SDL_Renderer* renderer, bool isMovingRight, bool isMovingLeft) {
        SDL_Rect dst = {static_cast<int>(pos.x), static_cast<int>(pos.y), w, h}; // Destination rectangle
        SDL_RendererFlip flip = SDL_FLIP_NONE; // Default no flip
        if (isMovingLeft && runTextures[0]) {
            SDL_RenderCopyEx(renderer, runTextures[curFrame], nullptr, &dst, 0.0, nullptr, flip); // Render left-facing run animation
            lastDirection = -1; // Update last direction
        } else if (isMovingRight && runTextures[0]) {
            SDL_RenderCopyEx(renderer, runTextures[curFrame], nullptr, &dst, 0.0, nullptr, SDL_FLIP_HORIZONTAL); // Render right-facing run animation
            lastDirection = 1;
        } else if (!onGround && standTextures[0]) {
            flip = (lastDirection != -1) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE; // Flip based on last direction
            SDL_RenderCopyEx(renderer, standTextures[curFrame], nullptr, &dst, 0.0, nullptr, flip); // Render airborne stand animation
        } else if (onGround && standTextures[0]) {
            flip = (lastDirection != -1) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
            SDL_RenderCopyEx(renderer, standTextures[curFrame], nullptr, &dst, 0.0, nullptr, flip); // Render grounded stand animation
        } else if (runTextures[0]) {
            SDL_RenderCopyEx(renderer, runTextures[curFrame], nullptr, &dst, 0.0, nullptr, flip); // Fallback to run animation
        }
    }

    // Update entity position, velocity, and collisions
    virtual void update(Terrain* terrain, bool hasInput, bool isMovingRight, bool isMovingLeft) {
        if (gravity) vel.y += GRAVITY; // Apply gravity
        if (onGround && friction && !hasInput) vel.x *= FRICTION; // Apply friction if no input
        pos.add(vel); // Update position based on velocity

        onGround = false; // Reset ground state
        if (grounded(terrain) && vel.y >= 0) { // Check if entity is on ground
            vel.y = 0; // Stop vertical movement
            onGround = true;
            for (const auto& platform : terrain->platforms) {
                SDL_Rect platformRect = {platform.x * TILE_SIZE, platform.y * TILE_SIZE, platform.width * TILE_SIZE, platform.height * TILE_SIZE};
                if (pos.x + w > platformRect.x && pos.x < platformRect.x + platformRect.w &&
                    pos.y + h >= platformRect.y && pos.y + h <= platformRect.y + 10) {
                    pos.y = platformRect.y - h; // Snap to platform top
                    break;
                }
            }
        }

        if (ceilingCol(terrain) && vel.y < 0) { // Check for ceiling collision
            vel.y = 0; // Stop upward movement
            for (const auto& platform : terrain->platforms) {
                SDL_Rect platformRect = {platform.x * TILE_SIZE, platform.y * TILE_SIZE, platform.width * TILE_SIZE, platform.height * TILE_SIZE};
                if (pos.x + w > platformRect.x && pos.x < platformRect.x + platformRect.w &&
                    pos.y <= platformRect.y + platformRect.h && pos.y >= platformRect.y) {
                    pos.y = platformRect.y + platformRect.h; // Snap to platform bottom
                    break;
                }
            }
        }

        if (leftCol(terrain) && vel.x < 0 && !ceilingCol(terrain)) { // Check for left wall collision
            vel.x = 0; // Stop leftward movement
            for (const auto& platform : terrain->platforms) {
                SDL_Rect platformRect = {platform.x * TILE_SIZE, platform.y * TILE_SIZE, platform.width * TILE_SIZE, platform.height * TILE_SIZE};
                if (pos.y + h > platformRect.y && pos.y < platformRect.y + platformRect.h &&
                    pos.x <= platformRect.x + platformRect.w && pos.x >= platformRect.x) {
                    pos.x = platformRect.x + platformRect.w; // Snap to right side of platform
                    break;
                }
            }
        }
        if (rightCol(terrain) && vel.x > 0 && !ceilingCol(terrain)) { // Check for right wall collision
            vel.x = 0; // Stop rightward movement
            for (const auto& platform : terrain->platforms) {
                SDL_Rect platformRect = {platform.x * TILE_SIZE, platform.y * TILE_SIZE, platform.width * TILE_SIZE, platform.height * TILE_SIZE};
                if (pos.y + h > platformRect.y && pos.y < platformRect.y + platformRect.h &&
                    pos.x + w >= platformRect.x && pos.x + w <= platformRect.x + platformRect.w) {
                    pos.x = platformRect.x - w; // Snap to left side of platform
                    break;
                }
            }
        }

        // Keep entity within screen bounds
        if (pos.x <= 10) { pos.x = 10; vel.x = 10; }
        else if (pos.x > SCREEN_WIDTH - w -30) { pos.x = SCREEN_WIDTH - w -60; vel.x = SCREEN_WIDTH - w -60; }
        else if (pos.y <= 0) { pos.y = 0; vel.y = 0; }
        else if (pos.y >= SCREEN_HEIGHT -h -50 ) { pos.y = SCREEN_HEIGHT -h -60; vel.y = SCREEN_HEIGHT -h -60; }

        // Update animation frame
        float deltaTime = SDL_GetTicks() / 1000.0f - frameTime;
        frameTime += deltaTime;
        if (frameTime >= frameSpeed) {
            frameTime = 0.0f;
            if (isMovingRight || isMovingLeft || !onGround) {
                curFrame = (curFrame + 1) % 10; // Cycle through run animation
            } else if (onGround) {
                curFrame = (curFrame + 1) % 12; // Cycle through stand animation
            }
        }
    }

    // Apply acceleration to velocity
    void accelerate(float x, float y) {
        vel.x += x;
        vel.y += y;
        if (vel.x > MAX_SPEED) vel.x = MAX_SPEED; // Cap horizontal speed
        if (vel.x < -MAX_SPEED) vel.x = -MAX_SPEED;
    }

    // Set collision box dimensions
    void setCol(float x, float y) {
        col.set(x, y);
    }

    // Check if entity is grounded
    bool grounded(Terrain* terrain) {
        return terrain->getSolid((int)(pos.x + col.x - 1), (int)(pos.y + col.y)) ||
               terrain->getSolid((int)(pos.x + 1), (int)(pos.y + col.y));
    }

    // Check for ceiling collision
    bool ceilingCol(Terrain* terrain) {
        return terrain->getSolid((int)(pos.x + col.x - 1), (int)(pos.y)) ||
               terrain->getSolid((int)(pos.x + 1), (int)(pos.y));
    }

    // Check for left wall collision
    bool leftCol(Terrain* terrain) {
        if (vel.x >= 0) return false;
        return terrain->getSolid((int)(pos.x), (int)(pos.y + col.y - 1)) ||
               terrain->getSolid((int)(pos.x), (int)(pos.y));
    }

    // Check for right wall collision
    bool rightCol(Terrain* terrain) {
        if (vel.x <= 0) return false;
        return terrain->getSolid((int)(pos.x + col.x), (int)(pos.y + col.y - 1)) ||
               terrain->getSolid((int)(pos.x + col.x), (int)(pos.y));
    }

    // Get the entity's bounding rectangle
    SDL_Rect getRect() const { return {static_cast<int>(pos.x), static_cast<int>(pos.y), w, h}; }
};

// Zombie class, inherits from PhysicsEntity
class Zombie : public PhysicsEntity {
public:
    enum Type { ATTACK, TANK }; // Zombie types: fast attacker or slow tank
    Type type; // Current zombie type
    float speed; // Movement speed
    int damage; // Damage dealt to player
    double lastDamageTime; // Time of last damage dealt

    // Constructor initializing zombie with position, size, texture, and type
    Zombie(float x, float y, int w_, int h_, SDL_Texture* tex, Type t)
    : PhysicsEntity(x, y, w_, h_, new SDL_Texture*[10]{tex, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr},
                               new SDL_Texture*[12]{tex, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr}),
      type(t), lastDamageTime(0.0) {
    speed = (type == ATTACK) ? 2.0f : 0.5f; // Set speed based on type
    damage = (type == ATTACK) ? 5 : 10; // Set damage based on type
    health = (type == ATTACK) ? 50 : 100; // Set health based on type
    }

    // Update zombie to chase player and handle physics
    void update(Terrain* terrain, const PhysicsEntity& player, bool isMovingRight, bool isMovingLeft) {
        float dx = player.pos.x - pos.x; // Distance to player
        if (std::abs(dx) > 5.0f) {
            vel.x = (dx > 0 ? speed : -speed); // Move toward player
        } else {
            vel.x = 0; // Stop if close to player
        }
        if (gravity) vel.y += GRAVITY; // Apply gravity
        if (onGround && friction && !vel.x) vel.x *= FRICTION; // Apply friction
        pos.add(vel); // Update position

        onGround = false; // Reset ground state
        if (grounded(terrain) && vel.y >= 0) { // Check if on ground
            vel.y = 0;
            onGround = true;
            for (const auto& platform : terrain->platforms) {
                SDL_Rect platformRect = {platform.x * TILE_SIZE, platform.y * TILE_SIZE, platform.width * TILE_SIZE, platform.height * TILE_SIZE};
                if (pos.x + w > platformRect.x && pos.x < platformRect.x + platformRect.w &&
                    pos.y + h >= platformRect.y && pos.y + h <= platformRect.y + 10) {
                    pos.y = platformRect.y - h; // Snap to platform top
                    break;
                }
            }
        }

        if (ceilingCol(terrain) && vel.y < 0) { // Check for ceiling collision
            vel.y = 0;
            for (const auto& platform : terrain->platforms) {
                SDL_Rect platformRect = {platform.x * TILE_SIZE, platform.y * TILE_SIZE, platform.width * TILE_SIZE, platform.height * TILE_SIZE};
                if (pos.x + w > platformRect.x && pos.x < platformRect.x + platformRect.w &&
                    pos.y <= platformRect.y + platformRect.h && pos.y >= platformRect.y) {
                    pos.y = platformRect.y + platformRect.h; // Snap to platform bottom
                    break;
                }
            }
        }

        if (leftCol(terrain) && vel.x < 0 && !ceilingCol(terrain)) { // Check for left wall collision
            vel.x = 0;
            for (const auto& platform : terrain->platforms) {
                SDL_Rect platformRect = {platform.x * TILE_SIZE, platform.y * TILE_SIZE, platform.width * TILE_SIZE, platform.height * TILE_SIZE};
                if (pos.y + h > platformRect.y && pos.y < platformRect.y + platformRect.h &&
                    pos.x <= platformRect.x + platformRect.w && pos.x >= platformRect.x) {
                    pos.x = platformRect.x + platformRect.w; // Snap to right side
                    break;
                }
            }
        }
        if (rightCol(terrain) && vel.x > 0 && !ceilingCol(terrain)) { // Check for right wall collision
            vel.x = 0;
            for (const auto& platform : terrain->platforms) {
                SDL_Rect platformRect = {platform.x * TILE_SIZE, platform.y * TILE_SIZE, platform.width * TILE_SIZE, platform.height * TILE_SIZE};
                if (pos.y + h > platformRect.y && pos.y < platformRect.y + platformRect.h &&
                    pos.x + w >= platformRect.x && pos.x + w <= platformRect.x + platformRect.w) {
                    pos.x = platformRect.x - w; // Snap to left side
                    break;
                }
            }
        }

        // Keep zombie within screen bounds
        if (pos.x <= 10) { pos.x = 10; vel.x = 10; }
        else if (pos.x > SCREEN_WIDTH - w -30) { pos.x = SCREEN_WIDTH - w -60; vel.x = SCREEN_WIDTH - w -60; }
        else if (pos.y <= 0) { pos.y = 0; vel.y = 0; }
        else if (pos.y >= SCREEN_HEIGHT -h -50 ) { pos.y = SCREEN_HEIGHT -h -50; vel.y = SCREEN_HEIGHT -h -50; }
    }

    // Render zombie with health bar
    void render(SDL_Renderer* renderer, bool isMovingRight, bool isMovingLeft) override {
        if (!renderer) {
            std::cerr << "Renderer is null in Zombie::render\n"; // Error if renderer is null
            return;
        }
        if (!runTextures[0]) {
            std::cerr << "Zombie texture is null in Zombie::render\n"; // Error if texture is null
            return;
        }
        SDL_Rect dst = {static_cast<int>(pos.x), static_cast<int>(pos.y), w, h}; // Destination rectangle
        SDL_RendererFlip flip = (vel.x >= 0) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE; // Flip based on movement
        if (SDL_RenderCopyEx(renderer, runTextures[0], nullptr, &dst, 0.0, nullptr, flip) != 0) {
            std::cerr << "SDL_RenderCopyEx failed: " << SDL_GetError() << "\n"; // Error on render failure
        }
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Set color for health bar
        SDL_Rect healthBar = {static_cast<int>(pos.x), static_cast<int>(pos.y - 10), health / 2, 5}; // Health bar rectangle
        SDL_RenderFillRect(renderer, &healthBar); // Render health bar
    }
};

// Food class, inherits from PhysicsEntity
class Food : public Zombie {
public:
    double spawnTime; // Time when food was spawned
    static const int HEALTH_RESTORE = 20; // Health restored when collected
    static constexpr double LIFETIME = 10.0; // Time before food despawns

    // Constructor initializing food with position, size, and texture
    Food(float x, float y, int w_, int h_, SDL_Texture* tex)
        : Zombie(x, y, w_, h_, tex, Zombie::ATTACK), spawnTime(SDL_GetTicks() / 1000.0) {
        health = 0; // Food has no health
        friction = false; // No friction for food
    }

    // Update food position and physics
    void update(Terrain* terrain, bool hasInput) {
        if (gravity) vel.y += GRAVITY; // Apply gravity
        if (onGround && friction && !vel.x) vel.x *= FRICTION; // Apply friction
        pos.add(vel); // Update position

        onGround = false; // Reset ground state
        if (grounded(terrain) && vel.y >= 0) { // Check if on ground
            vel.y = 0;
            onGround = true;
            for (const auto& platform : terrain->platforms) {
                SDL_Rect platformRect = {platform.x * TILE_SIZE, platform.y * TILE_SIZE, platform.width * TILE_SIZE, platform.height * TILE_SIZE};
                if (pos.x + w > platformRect.x && pos.x < platformRect.x + platformRect.w &&
                    pos.y + h >= platformRect.y && pos.y + h <= platformRect.y + 10) {
                    pos.y = platformRect.y - h; // Snap to platform top
                    break;
                }
            }
        }

        if (ceilingCol(terrain) && vel.y < 0) { // Check for ceiling collision
            vel.y = 0;
            for (const auto& platform : terrain->platforms) {
                SDL_Rect platformRect = {platform.x * TILE_SIZE, platform.y * TILE_SIZE, platform.width * TILE_SIZE, platform.height * TILE_SIZE};
                if (pos.x + w > platformRect.x && pos.x < platformRect.x + platformRect.w &&
                    pos.y <= platformRect.y + platformRect.h && pos.y >= platformRect.y) {
                    pos.y = platformRect.y + platformRect.h; // Snap to platform bottom
                    break;
                }
            }
        }

        if (leftCol(terrain) && vel.x < 0 && !ceilingCol(terrain)) { // Check for left wall collision
            vel.x = 0;
            for (const auto& platform : terrain->platforms) {
                SDL_Rect platformRect = {platform.x * TILE_SIZE, platform.y * TILE_SIZE, platform.width * TILE_SIZE, platform.height * TILE_SIZE};
                if (pos.y + h > platformRect.y && pos.y < platformRect.y + platformRect.h &&
                    pos.x <= platformRect.x + platformRect.w && pos.x >= platformRect.x) {
                    pos.x = platformRect.x + platformRect.w; // Snap to right side
                    break;
                }
            }
        }
        if (rightCol(terrain) && vel.x > 0 && !ceilingCol(terrain)) { // Check for right wall collision
            vel.x = 0;
            for (const auto& platform : terrain->platforms) {
                SDL_Rect platformRect = {platform.x * TILE_SIZE, platform.y * TILE_SIZE, platform.width * TILE_SIZE, platform.height * TILE_SIZE};
                if (pos.y + h > platformRect.y && pos.y < platformRect.y + platformRect.h &&
                    pos.x + w >= platformRect.x && pos.x + w <= platformRect.x + platformRect.w) {
                    pos.x = platformRect.x - w; // Snap to left side
                    break;
                }
            }
        }

        // Keep food within screen bounds
        if (pos.x < 0) { pos.x = 0; vel.x = 0; }
        if (pos.x > SCREEN_WIDTH - w) { pos.x = SCREEN_WIDTH - w; vel.x = 0; }
        if (pos.y < 0) { pos.y = 0; vel.y = 0; }
        if (pos.y > SCREEN_HEIGHT) { pos.y = SCREEN_HEIGHT - h; vel.y = 0; }
    }
};

// Structure to store game state for saving/loading
struct GameState {
    Vector2D playerPos; // Player position
    Vector2D playerVel; // Player velocity
    int playerHealth; // Player health
    int score; // Current score
    double startTime; // Game start time
    bool isValid; // Whether the state is valid
    std::vector<std::tuple<float, float, Zombie::Type>> zombies; // Store zombie position and type
    int wave; // Current wave
    int zombiesToSpawn; // Number of zombies left to spawn
    int waveZombiesRemaining; // Zombies remaining in current wave
    int weatherType; // Current weather state (DAY or NIGHT)
    GameState() : isValid(false), wave(1), zombiesToSpawn(15), waveZombiesRemaining(0), weatherType(0) {} // Default constructor
};

// Save game state to a binary file
void saveGameState(const GameState& state, const std::string& filename) {
    std::ofstream outFile(filename, std::ios::binary);
    if (outFile.is_open()) {
        outFile.write(reinterpret_cast<const char*>(&state.playerPos), sizeof(Vector2D)); // Save player position
        outFile.write(reinterpret_cast<const char*>(&state.playerVel), sizeof(Vector2D)); // Save player velocity
        outFile.write(reinterpret_cast<const char*>(&state.playerHealth), sizeof(int)); // Save player health
        outFile.write(reinterpret_cast<const char*>(&state.score), sizeof(int)); // Save score
        outFile.write(reinterpret_cast<const char*>(&state.startTime), sizeof(double)); // Save start time
        outFile.write(reinterpret_cast<const char*>(&state.isValid), sizeof(bool)); // Save validity flag
        size_t zombieCount = state.zombies.size();
        outFile.write(reinterpret_cast<const char*>(&zombieCount), sizeof(size_t)); // Save number of zombies
        for (const auto& zombie : state.zombies) {
            float x = std::get<0>(zombie);
            float y = std::get<1>(zombie);
            int type = static_cast<int>(std::get<2>(zombie));
            outFile.write(reinterpret_cast<const char*>(&x), sizeof(float)); // Save zombie x position
            outFile.write(reinterpret_cast<const char*>(&y), sizeof(float)); // Save zombie y position
            outFile.write(reinterpret_cast<const char*>(&type), sizeof(int)); // Save zombie type
        }
        outFile.write(reinterpret_cast<const char*>(&state.wave), sizeof(int)); // Save wave number
        outFile.write(reinterpret_cast<const char*>(&state.zombiesToSpawn), sizeof(int)); // Save zombies to spawn
        outFile.write(reinterpret_cast<const char*>(&state.waveZombiesRemaining), sizeof(int)); // Save remaining zombies
        outFile.write(reinterpret_cast<const char*>(&state.weatherType), sizeof(int)); // Save weather state
        outFile.close();
    }
}

// Load game state from a binary file
GameState loadGameState(const std::string& filename) {
    GameState state;
    std::ifstream inFile(filename, std::ios::binary);
    if (inFile.is_open()) {
        inFile.read(reinterpret_cast<char*>(&state.playerPos), sizeof(Vector2D)); // Load player position
        inFile.read(reinterpret_cast<char*>(&state.playerVel), sizeof(Vector2D)); // Load player velocity
        inFile.read(reinterpret_cast<char*>(&state.playerHealth), sizeof(int)); // Load player health
        inFile.read(reinterpret_cast<char*>(&state.score), sizeof(int)); // Load score
        inFile.read(reinterpret_cast<char*>(&state.startTime), sizeof(double)); // Load start time
        inFile.read(reinterpret_cast<char*>(&state.isValid), sizeof(bool)); // Load validity flag
        size_t zombieCount;
        inFile.read(reinterpret_cast<char*>(&zombieCount), sizeof(size_t)); // Load number of zombies
        state.zombies.resize(zombieCount);
        for (size_t i = 0; i < zombieCount; ++i) {
            float x, y;
            int type;
            inFile.read(reinterpret_cast<char*>(&x), sizeof(float)); // Load zombie x position
            inFile.read(reinterpret_cast<char*>(&y), sizeof(float)); // Load zombie y position
            inFile.read(reinterpret_cast<char*>(&type), sizeof(int)); // Load zombie type
            state.zombies[i] = std::make_tuple(x, y, static_cast<Zombie::Type>(type));
        }
        inFile.read(reinterpret_cast<char*>(&state.wave), sizeof(int)); // Load wave number
        inFile.read(reinterpret_cast<char*>(&state.zombiesToSpawn), sizeof(int)); // Load zombies to spawn
        inFile.read(reinterpret_cast<char*>(&state.waveZombiesRemaining), sizeof(int)); // Load remaining zombies
        inFile.read(reinterpret_cast<char*>(&state.weatherType), sizeof(int)); // Load weather state
        inFile.close();
        state.isValid = true; // Mark state as valid
    }
    return state;
}

// Save high score to a binary file
void saveHighScore(int score, const std::string& filename) {
    std::ofstream outFile(filename, std::ios::binary);
    if (outFile.is_open()) {
        outFile.write(reinterpret_cast<const char*>(&score), sizeof(int)); // Save score
        outFile.close();
    }
}

// Load high score from a binary file
int loadHighScore(const std::string& filename) {
    int highScore = 0;
    std::ifstream inFile(filename, std::ios::binary);
    if (inFile.is_open()) {
        inFile.read(reinterpret_cast<char*>(&highScore), sizeof(int)); // Load score
        inFile.close();
    }
    return highScore;
}

// Load animation textures for player
void loadAnimationTextures(int numFrames, SDL_Texture* textures[], const std::string& pathPrefix, SDL_Renderer* renderer) {
    for (int i = 0; i < numFrames; i++) {
        std::string filename = pathPrefix + std::to_string(i + 1) + ".png"; // Construct filename
        textures[i] = loadTexture(filename, renderer); // Load texture
        if (!textures[i]) {
            std::cerr << "Failed to load animation texture: " << filename << "\n"; // Log error if loading fails
        }
    }
}

// Render text to a texture
SDL_Texture* renderText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, SDL_Color color) {
    if (text.empty() || text.find_first_not_of(' ') == std::string::npos) return nullptr; // Return null for empty text
    SDL_Surface* surf = TTF_RenderText_Solid(font, text.c_str(), color); // Render text to surface
    if (!surf) {
        std::cerr << "Text render error: " << TTF_GetError() << std::endl; // Log error if rendering fails
        return nullptr;
    }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf); // Create texture from surface
    SDL_FreeSurface(surf); // Free surface
    if (!tex) {
        std::cerr << "Texture creation error: " << SDL_GetError() << std::endl; // Log error if texture creation fails
    }
    return tex;
}

// Spawn a zombie at a random platform
void spawnZombie(std::vector<Zombie*>& zombies, const Terrain& terrain, SDL_Texture* attackTex, SDL_Texture* tankTex) {
    if (terrain.platforms.empty()) {
        std::cerr << "No platforms available for zombie spawning\n"; // Log error if no platforms
        return;
    }
    static std::random_device rd; // Random device for seeding
    static std::mt19937 gen(rd()); // Mersenne Twister generator
    std::uniform_int_distribution<> platformDist(0, terrain.platforms.size() - 1); // Random platform index
    std::uniform_int_distribution<> typeDist(0, 1); // Random zombie type (ATTACK or TANK)
    std::uniform_real_distribution<> xDist(0.0f, 1.0f); // Random x position on platform

    const auto& platform = terrain.platforms[platformDist(gen)]; // Select random platform
    float x = platform.x * TILE_SIZE + xDist(gen) * (platform.width * TILE_SIZE - 32); // Random x position
    float y = platform.y * TILE_SIZE - 32; // Position above platform
    Zombie::Type type = typeDist(gen) == 0 ? Zombie::ATTACK : Zombie::TANK; // Random type
    SDL_Texture* tex = (type == Zombie::ATTACK) ? attackTex : tankTex; // Select texture based on type
    if (!tex) {
        std::cerr << "Zombie texture is null, cannot spawn zombie at (" << x << ", " << y << ")\n"; // Log error if texture missing
        return;
    }
    std::cout << "Spawning zombie at (" << x << ", " << y << ") type: " << (type == Zombie::ATTACK ? "ATTACK" : "TANK") << "\n"; // Log spawn
    Zombie* newZombie = new Zombie(x, y, 32, 32, tex, type); // Create new zombie
    if (!newZombie) {
        std::cerr << "Failed to create zombie object\n"; // Log error if creation fails
        return;
    }
    zombies.push_back(newZombie); // Add to zombie list
}

// Spawn food with 50% chance at zombie's position
void spawnFood(std::vector<Food*>& foods, float x, float y, SDL_Texture* foodTex) {
    if (!foodTex) {
        std::cerr << "Food texture not loaded\n"; // Log error if texture missing
        return;
    }
    static std::random_device rd; // Random device for seeding
    static std::mt19937 gen(rd()); // Mersenne Twister generator
    std::uniform_real_distribution<> chance(0.0f, 1.0f); // Random chance for spawning
    if (chance(gen) < 0.5f) { // 50% chance to spawn
        foods.push_back(new Food(x, y, 16, 16, foodTex)); // Create new food
        std::cout << "Spawned food at (" << x << ", " << y << ")\n"; // Log spawn
    }
}

// Main game loop function
int RunMainGame(const std::string& playerName, bool loadSaved, SDL_Window* win, SDL_Renderer* ren) {
    // Load font for text rendering
    TTF_Font* font = TTF_OpenFont("arial.ttf", 24);
    if (!font) { 
        std::cerr << "Font load error: " << TTF_GetError() << "\n"; // Log font loading error
        return 0; // Return 0 for Game Over
    }

    // Initialize weather system for day/night transitions
    WeatherSystem weather(ren, SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!weather.isInitialized()) {
        std::cerr << "Failed to initialize WeatherSystem\n"; // Log weather initialization error
        TTF_CloseFont(font);
        return 0;
    }
    
    // Load textures for game elements
    SDL_Texture* bgTex = loadTexture("day.png", ren); // Background texture
    SDL_Texture* platformTex = loadTexture("tile_wall.png", ren); // Platform texture
    SDL_Texture* playerTex = loadTexture("player.png", ren); // Player texture
    SDL_Texture* attackZombieTex = loadTexture("attack_zombie.png", ren); // Attack zombie texture
    SDL_Texture* tankZombieTex = loadTexture("tank_zombie.png", ren); // Tank zombie texture
    SDL_Texture* foodTex = loadTexture("food.png", ren); // Food texture
    
    SDL_Texture* runTextures[10]; // Player run animation textures
    SDL_Texture* standTextures[12]; // Player stand animation textures
    loadAnimationTextures(10, runTextures, "player_run", ren); // Load run animations
    loadAnimationTextures(12, standTextures, "player_stand", ren); // Load stand animations

    // Check for missing critical textures
    if (!attackZombieTex || !tankZombieTex) {
        std::cerr << "Critical texture missing: attackTex or tankTex null. Check assets folder.\n";
    }
    bool texturesLoaded = true;
    for (int i = 0; i < 10; i++) if (!runTextures[i]) texturesLoaded = false; // Check run textures
    for (int i = 0; i < 12; i++) if (!standTextures[i]) texturesLoaded = false; // Check stand textures
    if (!bgTex || !platformTex || !attackZombieTex || !tankZombieTex || !foodTex || !texturesLoaded) {
        std::cerr << "Critical texture missing. Ensure all PNGs are in assets/ folder. Check console logs for details.\n";
        std::cerr << "Texture status: background=" << (bgTex ? "loaded" : "null") << ", platform=" << (platformTex ? "loaded" : "null")
                  << ", attackZombie=" << (attackZombieTex ? "loaded" : "null") << ", tankZombie=" << (tankZombieTex ? "loaded" : "null")
                  << ", food=" << (foodTex ? "loaded" : "null") << "\n"; // Log texture status
        // Cleanup resources
        weather.cleanup();
        TTF_CloseFont(font);
        TTF_Quit();
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
        SDL_Quit();
        return 1; // Return 1 for menu exit
    }
    
    // Define platforms for the game terrain
    std::vector<Platform> platforms = {
        {0, 17, 30, 2, platformTex}, // Ground level
        {2, 12, 8, 1, platformTex}, // Platform 1
        {15, 12, 8, 1, platformTex}, // Platform 2
        {10, 8, 5, 1, platformTex}, // Platform 3
        {2, 4, 8, 1, platformTex}, // Platform 4
        {15, 4, 8, 1, platformTex} // Platform 5
    };

    Terrain terrain(platforms); // Initialize terrain with platforms

    // Initialize player
    PhysicsEntity player(TILE_SIZE * 3.0f, TILE_SIZE * 10.0f - 48.0f, 48, 48, runTextures, standTextures);
    player.setCol(48, 48); // Set player collision box

    // Initialize game variables
    int score = 0; // Current score
    double startTime = SDL_GetTicks() / 1000.0; // Game start time
    std::vector<Zombie*> zombies; // List of active zombies
    std::vector<Food*> foods; // List of active food items
    bool attacking = false; // Whether player is attacking
    double lastAttackTime = 0.0; // Time of last attack
    const double ATTACK_COOLDOWN = 0.5; // Attack cooldown time
    const int MELEE_DAMAGE = 25; // Damage dealt by player attack
    const int MELEE_RANGE = 100; // Range of player attack
    const int MAX_ZOMBIES_ONSCREEN = 5; // Maximum zombies on screen
    const int ZOMBIES_PER_WAVE[] = {15, 20, 25, 30, 35}; // Zombies per wave
    const int TOTAL_WAVES = 5; // Total number of waves
    int wave = 1; // Current wave
    int zombiesToSpawn = ZOMBIES_PER_WAVE[0]; // Zombies left to spawn
    int waveZombiesRemaining = 0; // Zombies remaining in current wave

    // Load saved game state if requested
    if (loadSaved) {
        GameState state = loadGameState("savegame.dat");
        if (state.isValid) {
            player.pos = state.playerPos; // Restore player position
            player.vel = state.playerVel; // Restore player velocity
            player.health = state.playerHealth; // Restore player health
            score = state.score; // Restore score
            startTime = SDL_GetTicks() / 1000.0 - state.startTime; // Restore game time
            for (const auto& zombie : state.zombies) {
                float x = std::get<0>(zombie);
                float y = std::get<1>(zombie);
                Zombie::Type type = std::get<2>(zombie);
                SDL_Texture* tex = (type == Zombie::ATTACK) ? attackZombieTex : tankZombieTex;
                zombies.push_back(new Zombie(x, y, 32, 32, tex, type)); // Restore zombies
            }
            wave = state.wave; // Restore wave
            zombiesToSpawn = state.zombiesToSpawn; // Restore zombies to spawn
            waveZombiesRemaining = state.waveZombiesRemaining; // Restore remaining zombies
            weather.setType(state.weatherType); // Restore weather state
        }
    }

    int highScore = loadHighScore("highscore.dat"); // Load high score

    // Define game screen states
    enum GameScreenState { PLAYING, PAUSED, GAME_OVER, VICTORY };
    GameScreenState gameState = PLAYING; // Initial state

    bool running = true, jumping = false; // Game loop and jump flags
    SDL_Event e; // Event handling

    // Create text textures for UI
    SDL_Color white = {255, 255, 255, 255}; // White color for text
    SDL_Texture* pauseText = renderText(ren, font, "Paused", white);
    SDL_Texture* resumeText = renderText(ren, font, "Resume", white);
    SDL_Texture* saveText = renderText(ren, font, "Save Game", white);
    SDL_Texture* menuText = renderText(ren, font, "Back to Menu", white);
    SDL_Texture* nameText = renderText(ren, font, playerName, white);
    SDL_Texture* gameOverText = renderText(ren, font, "Game Over!", white);
    SDL_Texture* victoryText = renderText(ren, font, "Victory!", white);
    SDL_Texture* scoreTextGameOver = nullptr;
    SDL_Texture* highScoreText = nullptr;
    SDL_Texture* backText = renderText(ren, font, "Back to Menu", white);

    // Check for missing UI textures
    if (!pauseText || !resumeText || !saveText || !menuText || !nameText || !gameOverText || !victoryText || !backText) {
        std::cerr << "Failed to create pause menu, name, game over, or victory textures.\n"; // Log error
        for (auto zombie : zombies) delete zombie; // Cleanup zombies
        for (auto food : foods) delete food; // Cleanup food
        // Cleanup resources
        weather.cleanup();
        TTF_CloseFont(font); 
        SDL_DestroyTexture(bgTex); SDL_DestroyTexture(platformTex); SDL_DestroyTexture(playerTex);
        SDL_DestroyTexture(attackZombieTex); SDL_DestroyTexture(tankZombieTex); SDL_DestroyTexture(foodTex);
        SDL_DestroyTexture(pauseText); SDL_DestroyTexture(resumeText); SDL_DestroyTexture(saveText);
        SDL_DestroyTexture(menuText); SDL_DestroyTexture(nameText); SDL_DestroyTexture(gameOverText);
        SDL_DestroyTexture(victoryText); SDL_DestroyTexture(backText);
        return 0; // Return 0 for Game Over
    }

    // Define UI rectangle positions
    SDL_Rect resumeRect = {300, 200, 200, 50}; // Resume button
    SDL_Rect saveRect = {300, 280, 200, 50}; // Save button
    SDL_Rect menuRect = {300, 360, 200, 50}; // Menu button
    SDL_Rect backRect = {300, 360, 200, 50}; // Back button for game over/victory

    // Button states for UI interaction
    enum ButtonState { NORMAL, HOVERED, PRESSED };
    ButtonState resumeState = NORMAL, saveState = NORMAL, menuState = NORMAL, backState = NORMAL;

    // Main game loop
    while (running) {
        // Handle events
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                running = false; // Exit on window close
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE && gameState == PLAYING) {
                gameState = PAUSED; // Pause game
            } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE && gameState == PAUSED) {
                gameState = PLAYING; // Resume game
            } else if (gameState == PLAYING) {
                if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE && player.onGround && !jumping) {
                    player.accelerate(0, JUMP_FORCE); // Player jumps
                    player.onGround = false; jumping = true;
                } else if (e.type == SDL_KEYUP && e.key.keysym.sym == SDLK_SPACE) {
                    jumping = false; // Stop jumping
                } else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_f) {
                    double currentTime = SDL_GetTicks() / 1000.0;
                    if (currentTime - lastAttackTime >= ATTACK_COOLDOWN) {
                        attacking = true; // Start attack
                        lastAttackTime = currentTime;
                    }
                }
            } else if (gameState == PAUSED) {
                if (e.type == SDL_MOUSEMOTION) {
                    int x = e.motion.x, y = e.motion.y;
                    // Update button states based on mouse position
                    resumeState = (x >= resumeRect.x && x <= resumeRect.x + resumeRect.w && y >= resumeRect.y && y <= resumeRect.y + resumeRect.h) ? HOVERED : NORMAL;
                    saveState = (x >= saveRect.x && x <= saveRect.x + saveRect.w && y >= saveRect.y && y <= saveRect.y + saveRect.h) ? HOVERED : NORMAL;
                    menuState = (x >= menuRect.x && x <= menuRect.x + menuRect.w && y >= menuRect.y && y <= menuRect.y + menuRect.h) ? HOVERED : NORMAL;
                } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                    // Set button state to pressed
                    if (resumeState == HOVERED) resumeState = PRESSED;
                    else if (saveState == HOVERED) saveState = PRESSED;
                    else if (menuState == HOVERED) menuState = PRESSED;
                } else if (e.type == SDL_MOUSEBUTTONUP) {
                    // Handle button actions
                    if (resumeState == PRESSED) {
                        gameState = PLAYING; // Resume game
                        resumeState = NORMAL;
                    } else if (saveState == PRESSED) {
                        GameState state;
                        state.playerPos = player.pos;
                        state.playerVel = player.vel;
                        state.playerHealth = player.health;
                        state.score = score;
                        state.startTime = (SDL_GetTicks() / 1000.0) - startTime;
                        state.isValid = true;
                        for (const auto& zombie : zombies) {
                            state.zombies.emplace_back(zombie->pos.x, zombie->pos.y, zombie->type); // Save zombies
                        }
                        state.wave = wave;
                        state.zombiesToSpawn = zombiesToSpawn;
                        state.waveZombiesRemaining = waveZombiesRemaining;
                        state.weatherType = weather.getType(); // Save weather state
                        saveGameState(state, "savegame.dat"); // Save game state
                        saveState = NORMAL;
                    } else if (menuState == PRESSED) {
                        running = false; // Exit to menu
                        menuState = NORMAL;
                    }
                }
            } else if (gameState == GAME_OVER || gameState == VICTORY) {
                if (e.type == SDL_MOUSEMOTION) {
                    int x = e.motion.x, y = e.motion.y;
                    backState = (x >= backRect.x && x <= backRect.x + backRect.w && y >= backRect.y && y <= backRect.y + backRect.h) ? HOVERED : NORMAL; // Update back button state
                } else if (e.type == SDL_MOUSEBUTTONDOWN) {
                    if (backState == HOVERED) backState = PRESSED; // Set back button to pressed
                } else if (e.type == SDL_MOUSEBUTTONUP) {
                    if (backState == PRESSED) {
                        running = false; // Exit to menu
                        backState = NORMAL;
                    }
                }
            }
        }

        if (gameState == PLAYING) {
            // Handle player input
            const Uint8* keys = SDL_GetKeyboardState(NULL);
            bool hasInput = false;
            bool isMovingRight = keys[SDL_SCANCODE_D];
            bool isMovingLeft = keys[SDL_SCANCODE_A];
            if (isMovingLeft) {
                player.accelerate(-PLAYER_ACCEL, 0); // Move left
                hasInput = true;
                std::cout << "Moving left\n"; // Log movement
            }
            if (isMovingRight) {
                player.accelerate(PLAYER_ACCEL, 0); // Move right
                hasInput = true;
                std::cout << "Moving right\n"; // Log movement
            }

            player.update(&terrain, hasInput, isMovingRight, isMovingLeft); // Update player
            
            double currentTime = SDL_GetTicks() / 1000.0; // Current time

            weather.update(currentTime); // Update weather system

            // Spawn zombies if needed
            if (zombiesToSpawn > 0 && zombies.size() < MAX_ZOMBIES_ONSCREEN) {
                spawnZombie(zombies, terrain, attackZombieTex, tankZombieTex); // Spawn a zombie
                zombiesToSpawn--;
                waveZombiesRemaining++;
            }

            SDL_Rect playerRect = player.getRect(); // Player's bounding rectangle
            std::vector<Zombie*> zombiesToDelete; // Zombies to remove
            for (auto& zombie : zombies) {
                zombie->update(&terrain, player, isMovingRight, isMovingLeft); // Update zombie
                SDL_Rect zombieRect = zombie->getRect(); // Zombie's bounding rectangle
                if (SDL_HasIntersection(&playerRect, &zombieRect)) { // Check collision with player
                    if (currentTime - zombie->lastDamageTime >= 1.0) {
                        player.health -= zombie->damage; // Damage player
                        zombie->lastDamageTime = currentTime;
                        if (player.health < 0) player.health = 0;
                    }
                }
                if (SDL_HasIntersection(&playerRect, &zombieRect)) { // Duplicate collision check (redundant)
                    if (currentTime - zombie->lastDamageTime >= 1.0) {
                        player.health -= zombie->damage;
                        zombie->lastDamageTime = currentTime;
                        if (player.health < 0) player.health = 0;
                    }
                }
                if (attacking) { // Handle player attack
                    SDL_Rect attackRect = {static_cast<int>(player.pos.x - MELEE_RANGE / 2 + player.w / 2),
                                        static_cast<int>(player.pos.y - MELEE_RANGE / 2 + player.h / 2),
                                        MELEE_RANGE, MELEE_RANGE}; // Attack hitbox
                    if (SDL_HasIntersection(&attackRect, &zombieRect)) {
                        zombie->health -= MELEE_DAMAGE; // Damage zombie
                        if (zombie->health <= 0) {
                            if (foodTex) {
                                spawnFood(foods, zombie->pos.x, zombie->pos.y, foodTex); // Spawn food on death
                            }
                            zombiesToDelete.push_back(zombie); // Mark for deletion
                            score += 100; // Increase score
                            waveZombiesRemaining--;
                        }
                    }
                }
            }
            // Remove dead zombies
            for (auto zombie : zombiesToDelete) {
                auto it = std::find(zombies.begin(), zombies.end(), zombie);
                if (it != zombies.end()) {
                    delete *it;
                    zombies.erase(it);
                }
            }
            attacking = false; // Reset attack state

            // Update and check food items
            for (auto it = foods.begin(); it != foods.end();) {
                Food* food = *it;
                food->update(&terrain, false); // Update food
                if (currentTime - food->spawnTime >= Food::LIFETIME) { // Check if food expired
                    delete food;
                    it = foods.erase(it);
                    continue;
                }
                SDL_Rect foodRect = food->getRect(); // Food's bounding rectangle
                if (SDL_HasIntersection(&playerRect, &foodRect)) { // Check collision with player
                    player.health += Food::HEALTH_RESTORE; // Restore health
                    if (player.health > 100) player.health = 100; // Cap health
                    delete food;
                    it = foods.erase(it); // Remove food
                    continue;
                }
                ++it;
            }

            // Check for wave completion
            if (waveZombiesRemaining == 0 && zombiesToSpawn == 0 && wave < TOTAL_WAVES) {
                wave++; // Advance to next wave
                zombiesToSpawn = ZOMBIES_PER_WAVE[wave - 1]; // Set zombies for new wave
            } else if (waveZombiesRemaining == 0 && zombiesToSpawn == 0 && wave == TOTAL_WAVES) {
                if (score > highScore) {
                    highScore = score;
                    saveHighScore(highScore, "highscore.dat"); // Save new high score
                }
                scoreTextGameOver = renderText(ren, font, "Your Score: " + std::to_string(score), white); // Create score text
                highScoreText = renderText(ren, font, "Highest Score: " + std::to_string(highScore), white); // Create high score text
                gameState = VICTORY; // Set victory state
            }

            // Check for game over conditions
            if (player.pos.y > SCREEN_HEIGHT || player.health <= 0) {
                if (score > highScore) {
                    highScore = score;
                    saveHighScore(highScore, "highscore.dat"); // Save new high score
                }
                scoreTextGameOver = renderText(ren, font, "Your Score: " + std::to_string(score), white); // Create score text
                highScoreText = renderText(ren, font, "Highest Score: " + std::to_string(highScore), white); // Create high score text
                gameState = GAME_OVER; // Set game over state
            }
        }

        // Clear renderer
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 255);
        SDL_RenderClear(ren);

        if (gameState != GAME_OVER && gameState != VICTORY) {
            // Render game elements
            SDL_Rect bgRect = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
            SDL_RenderCopy(ren, bgTex, nullptr, &bgRect); // Render background
            weather.render(ren, SCREEN_WIDTH, SCREEN_HEIGHT); // Render weather effects

            // Render platforms
            for (const auto& p : platforms) {
                for (int x = 0; x < p.width; ++x) {
                    for (int y = 0; y < p.height; ++y) {
                        SDL_Rect dst = {(p.x + x) * TILE_SIZE, (p.y + y) * TILE_SIZE, TILE_SIZE, TILE_SIZE};
                        SDL_RenderCopy(ren, p.texture, nullptr, &dst); // Render platform tiles
                    }
                }
            }
            const Uint8* keys = SDL_GetKeyboardState(NULL);
            bool isMovingRight = keys[SDL_SCANCODE_D];
            bool isMovingLeft = keys[SDL_SCANCODE_A];
            player.render(ren, isMovingRight, isMovingLeft); // Render player

            for (const auto& zombie : zombies) {
                zombie->render(ren, zombie->vel.x > 0, zombie->vel.x < 0); // Render zombies
            }
            for (const auto& food : foods) {
                food->render(ren, false, false); // Render food
            }
            if (attacking) {
                SDL_SetRenderDrawColor(ren, 255, 255, 0, 100); // Yellow for attack hitbox
                SDL_Rect attackRect = {static_cast<int>(player.pos.x - MELEE_RANGE / 2 + player.w / 2),
                                       static_cast<int>(player.pos.y - MELEE_RANGE / 2 + player.h / 2),
                                       MELEE_RANGE, MELEE_RANGE}; // Attack hitbox
                SDL_RenderFillRect(ren, &attackRect); // Render attack hitbox
            }
            SDL_SetRenderDrawColor(ren, 255, 0, 0, 255); // Red for health bar
            SDL_Rect healthBar = {10, 30, player.health * 2, 20}; // Player health bar
            SDL_RenderFillRect(ren, &healthBar); // Render health bar
            if (nameText) {
                SDL_Rect nameRect = {10, 5, 0, 0};
                SDL_QueryTexture(nameText, nullptr, nullptr, &nameRect.w, &nameRect.h);
                SDL_RenderCopy(ren, nameText, nullptr, &nameRect); // Render player name
            }
            SDL_Texture* scoreText = renderText(ren, font, "Score: " + std::to_string(score), white); // Create score text
            if (scoreText) {
                SDL_Rect scoreRect = {10, 60, 0, 0};
                SDL_QueryTexture(scoreText, nullptr, nullptr, &scoreRect.w, &scoreRect.h);
                SDL_RenderCopy(ren, scoreText, nullptr, &scoreRect); // Render score
                SDL_DestroyTexture(scoreText); // Free score text
            }
            SDL_Texture* waveText = renderText(ren, font, "Wave: " + std::to_string(wave) + "/5", white); // Create wave text
            if (waveText) {
                SDL_Rect waveRect = {10, 90, 0, 0};
                SDL_QueryTexture(waveText, nullptr, nullptr, &waveRect.w, &waveRect.h);
                SDL_RenderCopy(ren, waveText, nullptr, &waveRect); // Render wave
                SDL_DestroyTexture(waveText); // Free wave text
            }
        }

        if (gameState == PAUSED) {
            // Render pause menu
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 200); // Semi-transparent overlay
            SDL_Rect pauseOverlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
            SDL_RenderFillRect(ren, &pauseOverlay);

            SDL_Rect pauseTextRect = {300, 100, 200, 50};
            SDL_RenderCopy(ren, pauseText, nullptr, &pauseTextRect); // Render "Paused" text

            // Render resume button
            Uint8 resumeColor = (resumeState == HOVERED) ? 220 : (resumeState == PRESSED ? 180 : 200);
            roundedBoxRGBA(ren, resumeRect.x, resumeRect.y, resumeRect.x + resumeRect.w, resumeRect.y + resumeRect.h, 10, resumeColor, resumeColor, resumeColor, 255);
            SDL_RenderCopy(ren, resumeText, nullptr, &resumeRect);

            // Render save button
            Uint8 saveColor = (saveState == HOVERED) ? 220 : (saveState == PRESSED ? 180 : 200);
            roundedBoxRGBA(ren, saveRect.x, saveRect.y, saveRect.x + saveRect.w, saveRect.y + saveRect.h, 10, saveColor, saveColor, saveColor, 255);
            SDL_RenderCopy(ren, saveText, nullptr, &saveRect);

            // Render menu button
            Uint8 menuColor = (menuState == HOVERED) ? 220 : (menuState == PRESSED ? 180 : 200);
            roundedBoxRGBA(ren, menuRect.x, menuRect.y, menuRect.x + menuRect.w, menuRect.y + menuRect.h, 10, menuColor, menuColor, menuColor, 255);
            SDL_RenderCopy(ren, menuText, nullptr, &menuRect);
        } else if (gameState == GAME_OVER) {
            // Render game over screen
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 200); // Semi-transparent overlay
            SDL_Rect gameOverOverlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
            SDL_RenderFillRect(ren, &gameOverOverlay);

            SDL_Rect gameOverTextRect = {300, 100, 200, 50};
            SDL_RenderCopy(ren, gameOverText, nullptr, &gameOverTextRect); // Render "Game Over!" text

            if (scoreTextGameOver) {
                SDL_Rect scoreTextRect = {300, 180, 0, 0};
                SDL_QueryTexture(scoreTextGameOver, nullptr, nullptr, &scoreTextRect.w, &scoreTextRect.h);
                SDL_RenderCopy(ren, scoreTextGameOver, nullptr, &scoreTextRect); // Render score
            }

            if (highScoreText) {
                SDL_Rect highScoreTextRect = {300, 260, 0, 0};
                SDL_QueryTexture(highScoreText, nullptr, nullptr, &highScoreTextRect.w, &highScoreTextRect.h);
                SDL_RenderCopy(ren, highScoreText, nullptr, &highScoreTextRect); // Render high score
            }

            // Render back button
            Uint8 backColor = (backState == HOVERED) ? 220 : (backState == PRESSED ? 180 : 200);
            roundedBoxRGBA(ren, backRect.x, backRect.y, backRect.x + backRect.w, backRect.y + backRect.h, 10, backColor, backColor, backColor, 255);
            SDL_RenderCopy(ren, backText, nullptr, &backRect);
        } else if (gameState == VICTORY) {
            // Render victory screen
            SDL_SetRenderDrawColor(ren, 0, 0, 0, 200); // Semi-transparent overlay
            SDL_Rect victoryOverlay = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
            SDL_RenderFillRect(ren, &victoryOverlay);

            SDL_Rect victoryTextRect = {300, 100, 200, 50};
            SDL_RenderCopy(ren, victoryText, nullptr, &victoryTextRect); // Render "Victory!" text

            if (scoreTextGameOver) {
                SDL_Rect scoreTextRect = {300, 180, 0, 0};
                SDL_QueryTexture(scoreTextGameOver, nullptr, nullptr, &scoreTextRect.w, &scoreTextRect.h);
                SDL_RenderCopy(ren, scoreTextGameOver, nullptr, &scoreTextRect); // Render score
            }

            if (highScoreText) {
                SDL_Rect highScoreTextRect = {300, 260, 0, 0};
                SDL_QueryTexture(highScoreText, nullptr, nullptr, &highScoreTextRect.w, &highScoreTextRect.h);
                SDL_RenderCopy(ren, highScoreText, nullptr, &highScoreTextRect); // Render high score
            }

            // Render back button
            Uint8 backColor = (backState == HOVERED) ? 220 : (backState == PRESSED ? 180 : 200);
            roundedBoxRGBA(ren, backRect.x, backRect.y, backRect.x + backRect.w, backRect.y + backRect.h, 10, backColor, backColor, backColor, 255);
            SDL_RenderCopy(ren, backText, nullptr, &backRect);
        }

        SDL_RenderPresent(ren); // Present rendered frame
        SDL_Delay(16); // Cap frame rate to ~60 FPS
    }

    // Cleanup resources
    for (auto zombie : zombies) delete zombie;
    zombies.clear();
    for (auto food : foods) delete food;
    foods.clear();
    weather.cleanup(); // Clean up weather system
    TTF_CloseFont(font);
    SDL_DestroyTexture(bgTex);
    SDL_DestroyTexture(platformTex);
    SDL_DestroyTexture(playerTex);
    SDL_DestroyTexture(attackZombieTex);
    SDL_DestroyTexture(tankZombieTex);
    SDL_DestroyTexture(foodTex);
    SDL_DestroyTexture(pauseText);
    SDL_DestroyTexture(resumeText);
    SDL_DestroyTexture(saveText);
    SDL_DestroyTexture(menuText);
    SDL_DestroyTexture(nameText);
    SDL_DestroyTexture(gameOverText);
    SDL_DestroyTexture(victoryText);
    SDL_DestroyTexture(scoreTextGameOver);
    SDL_DestroyTexture(highScoreText);
    SDL_DestroyTexture(backText);

    // Return 0 for Game Over, 1 for menu exit or victory
    return (gameState == GAME_OVER) ? 0 : 1;
}