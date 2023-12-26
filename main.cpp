#include <SDL2/SDL.h>
#include <cstdio> 
#include <cstdint>
#include "Chip8.hpp"
#include "Chip8.cpp"

void setKeys(SDL_KeyboardEvent *key, uint8_t val);
void drawGraphics(SDL_Renderer *renderer);

Chip8 chip8;

const int PIXEL_SCALE = 10;
const int SCREEN_WIDTH = VIDEO_WIDTH * PIXEL_SCALE;
const int SCREEN_HEIGHT = VIDEO_HEIGHT * PIXEL_SCALE;


int main (int argc, char **argv) {
    if (argc != 2) {
        std::cout << "ERROR: PROPER USAGE IS: ./main ROM_NAME";
        return 1;
    }

    /*if (!chip8.loadROM(argv[1]))
        return 1;*/


    // Initialize SDL

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cout << "ERROR: SDL failed to initialize\nSDL Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(argv[1], SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    if (!window) {
        std::cout << "ERROR: Failed to open window\nSDL Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!renderer){
        std::cout << "ERROR: Failed to create renderer\nSDL Error: " << SDL_GetError() << std::endl;
        return 1;
    }


    // TODO: SLOW DOWN EMULATION TO 60 CYCLES PER SECOND AND ALSO MAKE THE SOUND_TIMER PLAY A SOUND
    // Main emulator loop
    bool running = true;
    while(running) {
        //chip8.emulateCycle();

        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT :
                    running = false;
                    break;

                case SDL_KEYDOWN :
                    //setKeys(&event.key, 1);

                case SDL_KEYUP :
                    //setKeys(&event.key, 0);
                    std::cout << SDL_GetKeyName( (&event.key)->keysym.sym ) << std::endl;

                default :
                    break;
            }
        }

        drawGraphics(renderer);
    }
    
    SDL_Quit();
    return 0;
}




// Set the keys that were pressed
// TODO: PRESSING KEYS IS FUCKED!!!!!!!!!!!!!!!!!!
void setKeys(SDL_KeyboardEvent *key, uint8_t val) {
    switch (key->keysym.sym) {
        case SDLK_0 :
            chip8.keypad[0x0] = val;
        
        case SDLK_1 :
            chip8.keypad[0x1] = val;

        case SDLK_2 :
            chip8.keypad[0x2] = val;

        case SDLK_3 :
            chip8.keypad[0x3] = val;

        case SDLK_4 :
            chip8.keypad[0x4] = val;

        case SDLK_5 :
            chip8.keypad[0x5] = val;

        case SDLK_6 :
            chip8.keypad[0x6] = val;

        case SDLK_7 :
            chip8.keypad[0x7] = val;

        case SDLK_8 :
            chip8.keypad[0x8] = val;

        case SDLK_9 :
            chip8.keypad[0x9] = val;

        case SDLK_a :
            chip8.keypad[0xA] = val;

        case SDLK_b :
            chip8.keypad[0xB] = val;

        case SDLK_c :
            chip8.keypad[0xC] = val;

        case SDLK_d :
            chip8.keypad[0xD] = val;

        case SDLK_e :
            chip8.keypad[0xE] = val;

        case SDLK_f :
            chip8.keypad[0xF] = val;
    }
}




// Draw each pixel to the screen
// Pixels are 10x10 in size
// Total screen resolution is 640x320
void drawGraphics(SDL_Renderer *renderer) {
    // Black background
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    // Draw each white pixel
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    for (int i = 0; i < VIDEO_WIDTH * VIDEO_HEIGHT; i++) {
        if (chip8.keypad[i]) {
            int x = (i % VIDEO_WIDTH) * PIXEL_SCALE;
            int y = (i / VIDEO_WIDTH) * PIXEL_SCALE;

            SDL_Rect r = {x, y, 10, 10};
            SDL_RenderDrawRect(renderer, &r);
            SDL_RenderFillRect(renderer, &r);
        }
    }

    // Render the current scene
    SDL_RenderPresent(renderer);
}


