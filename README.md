# Zombie Survival Simulator

## Overview

"Zombie Survival Simulator" is a 2D action-platformer game where you control a survivor navigating platforms, battling zombies, and collecting food to stay alive. Face waves of two types of zombies—fast-moving Attack Zombies and durable Tank Zombies—while managing your health and scoring points by defeating enemies. The game features a dynamic day/night cycle that changes the background every 30 seconds, adding an immersive atmosphere. Survive through five challenging waves to achieve victory, or save your progress to continue later. With retro pixel art and engaging mechanics, this game offers a thrilling survival experience for players of all ages who enjoy action-packed challenges.

## Requirements

- Linux (Ubuntu/Debian recommended)
- GCC/G++
- SDL2
- SDL2_ttf
- SDL2_image
- SDL2_gfx

Install SDL2 libraries on Ubuntu:
```bash
sudo apt-get install libsdl2-dev libsdl2-ttf-dev libsdl2-image-dev libsdl2-gfx-dev
```

## Build

Use the provided Makefile:

```bash
make tgame4
```
or to build and run:
```bash
make run
```

To clean up the executable:
```bash
make clean
```

## How to Play

- **Move:** A (left), D (right)
- **Jump:** Space
- **Attack:** F
- **Pause/Resume:** ESC
- **Save Game:** In pause menu, select "Save Game"
- **Return to Menu:** In pause menu or after Game Over/Victory

## Main Files

- `tgame4.cpp`: Main game logic, physics, rendering, save/load, high score.
- `utils.cpp`, `utils.h`: Utility functions, texture loading, etc.
- `Weather.cpp`, `Weather.h`: Weather and day/night effects.
- `Makefile`: Automates build/run/clean.

## Assets

Make sure all PNG images (player, zombie, food, tile, background, etc.) and the font `arial.ttf` are in the project directory or an `assets` folder.

## Notes

- If you get a "missing main" error, check that `tgame4.cpp` contains a `main` function.
- If the game doesn't run, check your asset and font paths.

## Contribution

Contributions, bug reports, and new ideas are welcome!

---
**Enjoy the game!**