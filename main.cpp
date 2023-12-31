#include <SDL2/SDL.h>
#include <cstdio> 
#include <cstdint>
#include <chrono>
#include "Chip8.hpp"
#include "Chip8.cpp"
//https://github.com/Timendus/chip8-test-suite

void setKeys(const Uint8 *keystate);
void drawGraphics(SDL_Renderer *renderer);

Chip8 chip8;

const int PIXEL_SCALE = 10;
const int SCREEN_WIDTH = VIDEO_WIDTH * PIXEL_SCALE;
const int SCREEN_HEIGHT = VIDEO_HEIGHT * PIXEL_SCALE;
const int TIMER_SPEED = 60;


int main (int argc, char **argv) {
    if (argc != 3) {
        std::cout << "ERROR: PROPER USAGE IS: ./Chip8 <ROM_NAME> <INSTRUCTIONS_PER_SECOND> ";
        return 1;
    }

    if (!chip8.loadROM(argv[1]))
        return 1;

    const int INSTRUCTIONS_PER_SECOND = std::stoi(argv[2]);

    //chip8.debug(D_MEM_ROM);

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



    // Delay/sound timers and cpu run at different speeds, so two seperate timers will be used
    auto timerStart = std::chrono::high_resolution_clock::now();        // Run at 60hz                
    auto cpuStart = std::chrono::high_resolution_clock::now();          // Speed will depend on the ROM and is configurable                                   
    
    // Main emulator loop
    const Uint8 *keystate;
    bool running = true;
    while(running) {

        // Handle events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT :
                    running = false;
                    break;

                default :
                    break;
            }
        }

        setKeys(keystate);

        // CPU delay is configurable based on the ROM being used, every (1 / INSTRUCTIONS_PER_SECOND) of a second run emulateCycle()
        auto cpuCurrent = std::chrono::high_resolution_clock::now();
        float cpuDiff = std::chrono::duration<float, std::chrono::seconds::period>(cpuCurrent - cpuStart).count();
        if (cpuDiff > 1.0 / INSTRUCTIONS_PER_SECOND) {
            cpuStart = cpuCurrent;
            chip8.emulateCycle();
        }

        // Timers update at 60hz, every 1/60 of a second run updateTimers()
        auto timerCurrent = std::chrono::high_resolution_clock::now();
        float timerDiff = std::chrono::duration<float, std::chrono::seconds::period>(timerCurrent - timerStart).count();
        if (timerDiff >= 1.0 / TIMER_SPEED) {
            timerStart = timerCurrent;
            chip8.updateTimers();
        }

        if (chip8.drawFlag) {
            drawGraphics(renderer);
            chip8.drawFlag = false;
        }

    }
    
    SDL_Quit();
    return 0;
}


// Set the keys that were pressed
void setKeys(const Uint8 *keystate) {
    keystate = SDL_GetKeyboardState(NULL);

    chip8.keypad[0x0] = keystate[SDL_SCANCODE_0];
    chip8.keypad[0x1] = keystate[SDL_SCANCODE_1];
    chip8.keypad[0x2] = keystate[SDL_SCANCODE_2];
    chip8.keypad[0x3] = keystate[SDL_SCANCODE_3];
    chip8.keypad[0x4] = keystate[SDL_SCANCODE_4];
    chip8.keypad[0x5] = keystate[SDL_SCANCODE_5];
    chip8.keypad[0x6] = keystate[SDL_SCANCODE_6];
    chip8.keypad[0x7] = keystate[SDL_SCANCODE_7];
    chip8.keypad[0x8] = keystate[SDL_SCANCODE_8];
    chip8.keypad[0x9] = keystate[SDL_SCANCODE_9];
    chip8.keypad[0xA] = keystate[SDL_SCANCODE_A];
    chip8.keypad[0xB] = keystate[SDL_SCANCODE_B];
    chip8.keypad[0xC] = keystate[SDL_SCANCODE_C];
    chip8.keypad[0xD] = keystate[SDL_SCANCODE_D];
    chip8.keypad[0xE] = keystate[SDL_SCANCODE_E];
    chip8.keypad[0xF] = keystate[SDL_SCANCODE_F];
        
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
        if (chip8.video[i]) {
            int x = (i % VIDEO_WIDTH) * PIXEL_SCALE;
            int y = (i / VIDEO_WIDTH) * PIXEL_SCALE;

            SDL_Rect rect = {x, y, 10, 10};
            SDL_RenderDrawRect(renderer, &rect);
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    // Render the current scene
    SDL_RenderPresent(renderer);
}


