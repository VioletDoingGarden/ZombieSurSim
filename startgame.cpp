#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_image.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <SDL2/SDL2_gfxPrimitives.h>
#include <random>
#include "utils.h"

// External function declaration for starting the main game
extern int RunMainGame(const std::string& playerName, bool loadSaved, SDL_Window* win, SDL_Renderer* ren);

// Screen dimensions and constants
const int SCREEN_WIDTH = 800; // Width of the game window
const int SCREEN_HEIGHT = 600; // Height of the game window
const int MAX_NAME_LENGTH = 10; // Maximum length of player name

// Function to render text to an SDL texture
SDL_Texture* renderText(const std::string& message, SDL_Color& color, TTF_Font* font, SDL_Renderer* renderer, int wrapLength = 0) {
    // Check for empty or whitespace-only text
    if (message.empty() || message.find_first_not_of(' ') == std::string::npos) return nullptr;
    // Render text to a surface, with wrapping if specified
    SDL_Surface* surface = wrapLength > 0 ?
        TTF_RenderText_Blended_Wrapped(font, message.c_str(), color, wrapLength) :
        TTF_RenderText_Solid(font, message.c_str(), color);
    if (!surface) {
        std::cerr << "Text render error: " << TTF_GetError() << std::endl; // Log error if surface creation fails
        return nullptr;
    }
    // Create texture from surface
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface); // Free the surface
    if (!texture) {
        std::cerr << "Texture creation error: " << SDL_GetError() << std::endl; // Log error if texture creation fails
    }
    return texture; // Return the created texture
}

// Function to check if a saved game file exists
bool hasSavedGame() {
    std::ifstream file("savegame.dat", std::ios::binary); // Attempt to open save file
    return file.good(); // Return true if file exists and is readable
}

int main(int argc, char* argv[]) {
    // Initialize SDL, SDL_ttf, and SDL_image
    if (SDL_Init(SDL_INIT_VIDEO) != 0 || TTF_Init() != 0 || !(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        std::cerr << "Init failed: " << SDL_GetError() << std::endl; // Log initialization error
        return 1; // Exit with error code
    }

    // Create game window
    SDL_Window* window = SDL_CreateWindow("Game Menu", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    // Create renderer for the window
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!window || !renderer) {
        std::cerr << "Window or renderer creation failed: " << SDL_GetError() << std::endl; // Log window/renderer creation error
        TTF_Quit(); IMG_Quit(); SDL_Quit(); // Cleanup SDL subsystems
        return 1; // Exit with error code
    }

    // Load background texture and fonts
    SDL_Texture* background = loadTexture("startgame.png", renderer); // Load menu background
    TTF_Font* titleFont = TTF_OpenFont("arial.ttf", 48); // Load font for title
    TTF_Font* font = TTF_OpenFont("arial.ttf", 36); // Load font for menu text
    if (!background || !titleFont || !font) {
        std::cerr << "Resource loading failed.\n"; // Log resource loading error
        TTF_CloseFont(titleFont); TTF_CloseFont(font); // Close fonts
        SDL_DestroyTexture(background); // Destroy background texture
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); // Destroy renderer and window
        TTF_Quit(); IMG_Quit(); SDL_Quit(); // Cleanup SDL subsystems
        return 1; // Exit with error code
    }

    // Define colors for text rendering
    SDL_Color black = {0, 0, 0, 255}; // Black color
    SDL_Color white = {255, 255, 255, 255}; // White color
    SDL_Color red = {255, 0, 0, 255}; // Red color for warnings

    // Create textures for menu text
    SDL_Texture* titleText = renderText("Zombie Survival Simulation", black, titleFont, renderer); // Title text
    SDL_Texture* startText = renderText("Start Game", black, font, renderer); // Start game button text
    SDL_Texture* continueText = renderText("Continue Game", black, font, renderer); // Continue game button text
    SDL_Texture* instructionText = renderText("Instruction", black, font, renderer); // Instruction button text
    SDL_Texture* backText = renderText("Back to Menu", white, font, renderer); // Back button text
    SDL_Texture* newGameText = renderText("New Game", black, font, renderer); // New game button text
    SDL_Texture* warningText = renderText("Only 10 Characters!", red, font, renderer); // Warning for name length
    SDL_Texture* noSaveWarningText = renderText("No previous game data!", red, font, renderer); // Warning for no save data

    // Define instruction message for the game
    std::string instructionMessage =
        "1. Use A and D to move the survivor.\nUse space to jump.\n2. Click F to use melee.\n3. There will be random food drops after killing zombies so you can heal yourself.\n4. Zombies will be more every wave and You need to survive 5 waves to win the game.\n5. Enjoy the game ! :)";
    SDL_Texture* instructionContent = renderText(instructionMessage, white, font, renderer, 700); // Instruction text with wrapping

    // Check for missing menu textures
    if (!titleText || !startText || !continueText || !instructionText || !backText || !newGameText || !instructionContent || !warningText || !noSaveWarningText) {
        std::cerr << "Failed to create menu textures.\n"; // Log texture creation error
        TTF_CloseFont(titleFont); TTF_CloseFont(font); // Close fonts
        SDL_DestroyTexture(background); SDL_DestroyTexture(titleText); SDL_DestroyTexture(startText); // Destroy textures
        SDL_DestroyTexture(continueText); SDL_DestroyTexture(instructionText); SDL_DestroyTexture(backText);
        SDL_DestroyTexture(newGameText); SDL_DestroyTexture(instructionContent); SDL_DestroyTexture(warningText);
        SDL_DestroyTexture(noSaveWarningText);
        SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); // Destroy renderer and window
        TTF_Quit(); IMG_Quit(); SDL_Quit(); // Cleanup SDL subsystems
        return 1; // Exit with error code
    }

    // Define rectangles for menu buttons
    SDL_Rect startRect = {300, 250, 200, 50}; // Start game button
    SDL_Rect continueRect = {300, 330, 200, 50}; // Continue game button
    SDL_Rect instructionRect = {300, 330, 200, 50}; // Instruction button
    SDL_Rect backRect = {SCREEN_WIDTH - 220, SCREEN_HEIGHT - 70, 200, 50}; // Back button
    SDL_Rect newGameRect = {300, 250, 200, 50}; // New game button
    SDL_Rect warningRect = {150, 360, 500, 50}; // Warning message for name length
    SDL_Rect noSaveWarningRect = {150, 400, 500, 50}; // Warning message for no save data

    // Enum for button states
    enum ButtonState { NORMAL, HOVERED, PRESSED };
    ButtonState startState = NORMAL, continueState = NORMAL, instructionState = NORMAL, backState = NORMAL, newGameState = NORMAL;

    // Enum for screen states
    enum ScreenState { MENU, INSTRUCTION, START_GAME, ENTER_NAME };
    ScreenState screenState = MENU; // Initial screen state

    bool running = true; // Main loop flag
    SDL_Event event; // Event handling
    std::string playerName = ""; // Player name input
    std::string lastPlayerName = ""; // Last entered player name for texture updates
    SDL_Texture* nameTexture = nullptr; // Texture for player name input
    bool showWarning = false; // Flag for name length warning
    bool isGameOver = true; // Flag to track game over state
    bool hasSaved = hasSavedGame(); // Check for saved game at startup

    SDL_StartTextInput(); // Enable text input for name entry

    

    // Main game loop
    while (running) {
        hasSaved = hasSavedGame() && !isGameOver; // Update save game availability

        // Handle events
        while (SDL_PollEvent(&event)) {
            int x = event.motion.x, y = event.motion.y; // Mouse position
            if (event.type == SDL_QUIT) {
                running = false; // Exit on window close
            } else if (event.type == SDL_MOUSEMOTION) {
                // Update button states based on mouse position
                if (screenState == MENU) {
                    startState = (x >= startRect.x && x <= startRect.x + startRect.w && y >= startRect.y && y <= startRect.y + startRect.h) ? HOVERED : NORMAL; // Start button
                    instructionState = (x >= instructionRect.x && x <= instructionRect.x + instructionRect.w && y >= instructionRect.y && y <= instructionRect.y + instructionRect.h) ? HOVERED : NORMAL; // Instruction button
                } else if (screenState == INSTRUCTION) {
                    backState = (x >= backRect.x && x <= backRect.x + backRect.w && y >= backRect.y && y <= backRect.y + backRect.h) ? HOVERED : NORMAL; // Back button
                } else if (screenState == START_GAME) {
                    newGameState = (x >= newGameRect.x && x <= newGameRect.x + newGameRect.w && y >= newGameRect.y && y <= newGameRect.y + newGameRect.h) ? HOVERED : NORMAL; // New game button
                    if (hasSaved) {
                        continueState = (x >= continueRect.x && x <= continueRect.x + continueRect.w && y >= continueRect.y && y <= continueRect.y + continueRect.h) ? HOVERED : NORMAL; // Continue button
                    } else {
                        continueState = NORMAL; // Disable continue button if no save
                    }
                    backState = (x >= backRect.x && x <= backRect.x + backRect.w && y >= backRect.y && y <= backRect.y + backRect.h) ? HOVERED : NORMAL; // Back button
                }
            } else if (event.type == SDL_MOUSEBUTTONDOWN) {
                // Set button states to pressed
                if (screenState == MENU && startState == HOVERED) startState = PRESSED;
                else if (screenState == MENU && instructionState == HOVERED) instructionState = PRESSED;
                else if (screenState == INSTRUCTION && backState == HOVERED) backState = PRESSED;
                else if (screenState == START_GAME && newGameState == HOVERED) newGameState = PRESSED;
                else if (screenState == START_GAME && continueState == HOVERED && hasSaved) continueState = PRESSED;
                else if (screenState == START_GAME && backState == HOVERED) backState = PRESSED;
            } else if (event.type == SDL_MOUSEBUTTONUP) {
                // Handle button actions
                if (screenState == MENU && startState == PRESSED) {
                    screenState = START_GAME; // Switch to start game screen
                    startState = NORMAL;
                } else if (screenState == MENU && instructionState == PRESSED) {
                    screenState = INSTRUCTION; // Switch to instruction screen
                    instructionState = NORMAL;
                } else if (screenState == INSTRUCTION && backState == PRESSED) {
                    screenState = MENU; // Return to main menu
                    backState = NORMAL;
                } else if (screenState == START_GAME && newGameState == PRESSED) {
                    screenState = ENTER_NAME; // Switch to name entry screen
                    playerName = ""; // Reset player name
                    lastPlayerName = ""; // Reset last player name
                    showWarning = false; // Reset warning flag
                    isGameOver = false; // Reset game over flag
                    if (nameTexture) {
                        SDL_DestroyTexture(nameTexture); // Destroy existing name texture
                        nameTexture = nullptr;
                    }
                    newGameState = NORMAL;
                } else if (screenState == START_GAME && continueState == PRESSED && hasSaved) {
                    SDL_StopTextInput(); // Disable text input
                    if (nameTexture) {
                        SDL_DestroyTexture(nameTexture); // Destroy name texture
                        nameTexture = nullptr;
                    }
                    // Run the main game with saved state
                    int exitStatus = RunMainGame(playerName, true, window, renderer);
                    SDL_StartTextInput(); // Re-enable text input
                    screenState = MENU; // Return to main menu
                    isGameOver = (exitStatus == 0); // Update game over status
                    continueState = NORMAL;
                } else if (screenState == START_GAME && backState == PRESSED) {
                    screenState = MENU; // Return to main menu
                    backState = NORMAL;
                }
            } else if (event.type == SDL_TEXTINPUT && screenState == ENTER_NAME) {
                // Handle text input for player name
                if (playerName.size() < MAX_NAME_LENGTH) {
                    playerName += event.text.text; // Append input character
                    showWarning = false; // Clear warning
                } else {
                    showWarning = true; // Show name length warning
                }
            } else if (event.type == SDL_KEYDOWN && screenState == ENTER_NAME) {
                // Handle keyboard input for name entry
                if (event.key.keysym.sym == SDLK_BACKSPACE && !playerName.empty()) {
                    playerName.pop_back(); // Remove last character
                    showWarning = false; // Clear warning
                } else if (event.key.keysym.sym == SDLK_RETURN && !playerName.empty()) {
                    SDL_StopTextInput(); // Disable text input
                    if (nameTexture) {
                        SDL_DestroyTexture(nameTexture); // Destroy name texture
                        nameTexture = nullptr;
                    }
                    // Run the main game with new game state
                    int exitStatus = RunMainGame(playerName, false, window, renderer);
                    SDL_StartTextInput(); // Re-enable text input
                    screenState = MENU; // Return to main menu
                    isGameOver = (exitStatus == 0); // Update game over status
                    showWarning = false; // Clear warning
                }
            }
        }

        // Check for invalid window or renderer
        if (!window || !renderer) {
            std::cerr << "Window or renderer became invalid.\n"; // Log error
            running = false; // Exit loop
            break;
        }

        // Clear renderer
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        // Render main menu
        if (screenState == MENU) {
            SDL_RenderCopy(renderer, background, nullptr, nullptr); // Render background
            SDL_Rect titleRect = {(SCREEN_WIDTH - 500) / 2, 150, 500, 50}; // Title position
            SDL_RenderCopy(renderer, titleText, nullptr, &titleRect); // Render title

            // Render start button
            Uint8 startColor = (startState == HOVERED) ? 220 : (startState == PRESSED ? 180 : 200); // Button color based on state
            roundedBoxRGBA(renderer, startRect.x, startRect.y, startRect.x + startRect.w, startRect.y + startRect.h, 10, startColor, startColor, startColor, 255);
            SDL_RenderCopy(renderer, startText, nullptr, &startRect); // Render start text

            // Render instruction button
            Uint8 instrColor = (instructionState == HOVERED) ? 220 : (instructionState == PRESSED ? 180 : 200);
            roundedBoxRGBA(renderer, instructionRect.x, instructionRect.y, instructionRect.x + instructionRect.w, instructionRect.y + instructionRect.h, 10, instrColor, instrColor, instrColor, 255);
            SDL_RenderCopy(renderer, instructionText, nullptr, &instructionRect); // Render instruction text
        } else if (screenState == INSTRUCTION) {
            // Render instruction screen
            SDL_Rect instRect = {50, 50, 700, 400}; // Instruction text position
            SDL_RenderCopy(renderer, instructionContent, nullptr, &instRect); // Render instruction content

            // Render back button
            Uint8 backColor = (backState == HOVERED) ? 140 : (backState == PRESSED ? 100 : 100);
            roundedBoxRGBA(renderer, backRect.x, backRect.y, backRect.x + backRect.w, backRect.y + backRect.h, 10, backColor, backColor, backColor, 255);
            SDL_RenderCopy(renderer, backText, nullptr, &backRect); // Render back text
        } else if (screenState == START_GAME) {
            // Render start game screen
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Clear screen
            SDL_RenderClear(renderer);

            SDL_Rect titleRect = {(SCREEN_WIDTH - 500) / 2, 150, 500, 50}; // Title position
            SDL_RenderCopy(renderer, titleText, nullptr, &titleRect); // Render title

            // Render new game button
            Uint8 newGameColor = (newGameState == HOVERED) ? 220 : (newGameState == PRESSED ? 180 : 200);
            roundedBoxRGBA(renderer, newGameRect.x, newGameRect.y, newGameRect.x + newGameRect.w, newGameRect.y + newGameRect.h, 10, newGameColor, newGameColor, newGameColor, 255);
            SDL_RenderCopy(renderer, newGameText, nullptr, &newGameRect); // Render new game text

            // Render continue button
            Uint8 continueColor = hasSaved ? (continueState == HOVERED ? 220 : (continueState == PRESSED ? 180 : 200)) : 100; // Dim if no save
            roundedBoxRGBA(renderer, continueRect.x, continueRect.y, continueRect.x + continueRect.w, continueRect.y + continueRect.h, 10, continueColor, continueColor, continueColor, 255);
            SDL_RenderCopy(renderer, continueText, nullptr, &continueRect); // Render continue text

            if (!hasSaved) {
                SDL_RenderCopy(renderer, noSaveWarningText, nullptr, &noSaveWarningRect); // Render no save warning
            }

            // Render back button
            Uint8 backColor = (backState == HOVERED) ? 140 : (backState == PRESSED ? 100 : 100);
            roundedBoxRGBA(renderer, backRect.x, backRect.y, backRect.x + backRect.w, backRect.y + backRect.h, 10, backColor, backColor, backColor, 255);
            SDL_RenderCopy(renderer, backText, nullptr, &backRect); // Render back text
        } else if (screenState == ENTER_NAME) {
            // Render name entry screen
            SDL_Rect instructionRect = {150, 260, 500, 50}; // Instruction text position
            SDL_Rect nameInputRect = {150, 310, 500, 50}; // Name input position

            // Render "Enter Name" instruction
            SDL_Texture* enterNameText = renderText("Enter Name (10 character limit!):", white, font, renderer);
            if (enterNameText) {
                SDL_RenderCopy(renderer, enterNameText, nullptr, &instructionRect); // Render instruction
                SDL_DestroyTexture(enterNameText); // Free temporary texture
            }

            // Update name texture if player name changed
            if (playerName != lastPlayerName) {
                if (nameTexture) SDL_DestroyTexture(nameTexture); // Destroy old texture
                nameTexture = renderText(playerName + "_", white, font, renderer); // Create new texture with cursor
                lastPlayerName = playerName; // Update last name
            }

            // Render player name input
            if (nameTexture) {
                int textWidth, textHeight;
                SDL_QueryTexture(nameTexture, nullptr, nullptr, &textWidth, &textHeight); // Get texture dimensions
                SDL_Rect textRect = {nameInputRect.x, nameInputRect.y + (nameInputRect.h - textHeight) / 2, textWidth, textHeight}; // Center text
                if (textRect.x + textWidth > nameInputRect.x + nameInputRect.w) {
                    textRect.w = nameInputRect.w - (textRect.x - nameInputRect.x); // Clip text to fit
                }
                SDL_RenderCopy(renderer, nameTexture, nullptr, &textRect); // Render name
            }

            if (showWarning) {
                SDL_RenderCopy(renderer, warningText, nullptr, &warningRect); // Render name length warning
            }
        }

        SDL_RenderPresent(renderer); // Present rendered frame
        SDL_Delay(16); // Cap frame rate to ~60 FPS
    }

    // Cleanup resources
    if (nameTexture) SDL_DestroyTexture(nameTexture); // Destroy name texture
    SDL_DestroyTexture(background); SDL_DestroyTexture(titleText); // Destroy menu textures
    SDL_DestroyTexture(startText); SDL_DestroyTexture(continueText); SDL_DestroyTexture(instructionText);
    SDL_DestroyTexture(instructionContent); SDL_DestroyTexture(backText);
    SDL_DestroyTexture(newGameText); SDL_DestroyTexture(warningText); SDL_DestroyTexture(noSaveWarningText);
    TTF_CloseFont(font); TTF_CloseFont(titleFont); // Close fonts
    SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); // Destroy renderer and window
    TTF_Quit(); IMG_Quit(); SDL_Quit(); // Cleanup SDL subsystems
    return 0; // Exit program
}