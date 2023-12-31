#include "Chip8.hpp"
#include <iostream> 
#include <iomanip> 
#include <cstdio> 
#include <cstdlib> 
#include <time.h>
//https://lazyfoo.net/tutorials/SDL/21_sound_effects_and_music/index.php

const unsigned int PROGRAM_START_ADDRES = 0x200;
const unsigned int FONTSET_SIZE = 80;
const unsigned int FONTSET_START_ADDRESS = 0x050;

// Represents the sprites 0-F
unsigned char chip8_fontset[FONTSET_SIZE] =
{ 
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};


Chip8::Chip8() {
    initialize();
}


// Initializes the registers and memory
void Chip8::initialize() {
    drawFlag = true;

    pc = PROGRAM_START_ADDRES;          // 0x200 is where Chip8 programs start
                                        // 0x000 to 0x1FF are where the original interpreter was located and should not be used

    sp = 0;                             // Reset stack pointer
    I = 0;                              // Reset index register 
    opcode = 0;                         // Reset current opcode 

    delay_timer = 0;                    // Reset delay timer
    sound_timer = 0;                    // Reset sound timer

    // Clear registers and keypad
    for (int i = 0; i < REGISTER_COUNT; i++) {
        keypad[i] = V[i] = 0;
    }

    // Clear video display
    for (int i = 0; i < VIDEO_WIDTH * VIDEO_HEIGHT; i++) {       
        video[i] = 0;
    }

    // Clear memory
    for (int i = 0; i < MEMORY_SIZE; i++) {
        memory[i] = 0;
    }

    // Load fonts (sprites 0-F) into memory from address 0x050 to 0x0A0
    for (int i = 0; i < FONTSET_SIZE; i++) {
        memory[FONTSET_START_ADDRESS + i] = chip8_fontset[i];
    }

    // Clear stack
    for (int i = 0; i < STACK_SIZE; i++) {
        stack[i] = 0;
    }


    // Set up function pointer table for opcodes
        OpcodeTable[0x0] = &Chip8::getTable0;
        OpcodeTable[0x1] = &Chip8::OP_1nnn;
        OpcodeTable[0x2] = &Chip8::OP_2nnn;
        OpcodeTable[0x3] = &Chip8::OP_3xkk;
        OpcodeTable[0x4] = &Chip8::OP_4xkk;
        OpcodeTable[0x5] = &Chip8::OP_5xy0;
        OpcodeTable[0x6] = &Chip8::OP_6xkk;
        OpcodeTable[0x7] = &Chip8::OP_7xkk;
        OpcodeTable[0x8] = &Chip8::getTable8;
        OpcodeTable[0x9] = &Chip8::OP_9xy0;
        OpcodeTable[0xA] = &Chip8::OP_Annn;
        OpcodeTable[0xB] = &Chip8::OP_Bnnn;
        OpcodeTable[0xC] = &Chip8::OP_Cxkk;
        OpcodeTable[0xD] = &Chip8::OP_Dxyn;
        OpcodeTable[0xE] = &Chip8::getTableE;
        OpcodeTable[0xF] = &Chip8::getTableF;


    // Initialize opcode tables 0, 8, and E with all NULL values
    for (int i = 0; i < 0xE + 1; i++) {
        OpcodeTable_0[i] = &Chip8::OP_NULL;
        OpcodeTable_8[i] = &Chip8::OP_NULL;
        OpcodeTable_E[i] = &Chip8::OP_NULL;
    }

    // Fill the proper indices with their opcodes
    OpcodeTable_0[0x0] = &Chip8::OP_00E0;
    OpcodeTable_0[0xE] = &Chip8::OP_00EE;

    OpcodeTable_8[0x0] = &Chip8::OP_8xy0;
    OpcodeTable_8[0x1] = &Chip8::OP_8xy1;
    OpcodeTable_8[0x2] = &Chip8::OP_8xy2;
    OpcodeTable_8[0x3] = &Chip8::OP_8xy3;
    OpcodeTable_8[0x4] = &Chip8::OP_8xy4;
    OpcodeTable_8[0x5] = &Chip8::OP_8xy5;
    OpcodeTable_8[0x6] = &Chip8::OP_8xy6;
    OpcodeTable_8[0x7] = &Chip8::OP_8xy7;
    OpcodeTable_8[0xE] = &Chip8::OP_8xyE;

    OpcodeTable_E[0xE] = &Chip8::OP_Ex9E;
    OpcodeTable_E[0x1] = &Chip8::OP_ExA1;


    // Initialize opcode table F with all NULL values
    for (int i = 0; i < 0x65 + 1; i++) {
        OpcodeTable_F[i] = &Chip8::OP_NULL;
    }

    // Fill the proper indices with their opcodes
    OpcodeTable_F[0x07] = &Chip8::OP_Fx07;
    OpcodeTable_F[0x0A] = &Chip8::OP_Fx0A;
    OpcodeTable_F[0x15] = &Chip8::OP_Fx15;
    OpcodeTable_F[0x18] = &Chip8::OP_Fx18;
    OpcodeTable_F[0x1E] = &Chip8::OP_Fx1E;
    OpcodeTable_F[0x29] = &Chip8::OP_Fx29;
    OpcodeTable_F[0x33] = &Chip8::OP_Fx33;
    OpcodeTable_F[0x55] = &Chip8::OP_Fx55;
    OpcodeTable_F[0x65] = &Chip8::OP_Fx65;

    // Initialize seed for random number generator
    srand(time(NULL));
}




// Index into OpcodeTable_0 based on the last hex digit of the opcode
void Chip8::getTable0() {
    int index = opcode & 0x000F;

    // Dereference OpcodeTable_0 and call the function at the index
    (this->*(OpcodeTable_0[index]))();
}


// Index into OpcodeTable_8 based on the last hex digit of the opcode
void Chip8::getTable8() {
    int index = opcode & 0x000F;

    // Dereference OpcodeTable_8 and call the function at the index
    (this->*(OpcodeTable_8[index]))();
}


// Index into OpcodeTable_E based on the last hex digit of the opcode
void Chip8::getTableE() {
    int index = opcode & 0x000F;

    // Dereference OpcodeTable_E and call the function at the index
    (this->*(OpcodeTable_E[index]))();
}


// Index into OpcodeTable_F based on the last two hex digits of the opcode
void Chip8::getTableF() {
    int index = opcode & 0x00FF;

    // Dereference OpcodeTable_0 and call the function at the index
    (this->*(OpcodeTable_F[index]))();
}




// Every cycle the CPU will fetch the opcode, decode the opcode, execute the opcode, and update timers
void Chip8::emulateCycle() {
    // Fetch Opcode
    opcode = (memory[pc] << 8) | memory[pc+1];      // Each opcode is 2 bytes long
                                                    // Need to merge the two halves in memory

    pc += 2;

    // Decode and Execute Opcode
    (this->*(OpcodeTable[(opcode & 0xF000) >> 12]))();
}




// Updates the delay timer and sound timer and plays a tone while the sound timer is > 0
void Chip8::updateTimers() {
    if (delay_timer) 
        delay_timer--;
    
    if (sound_timer) {
        std::cout << "BEEP" << std::endl;
        sound_timer--;
    }
}




// Load a program into memory starting from memory address 0x200
bool Chip8::loadROM(const char *filename) {
    initialize();

    FILE *fp;
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        std::cout << "ERROR: Could not open file" << std::endl << std::endl;
        return false;
    }

    // Obtain size of file
    long fsize;
    fseek(fp , 0 , SEEK_END);
    fsize = ftell(fp);
    rewind(fp);

    // If file is larger than memory allows, throw an error
    if (fsize > (MEMORY_SIZE - 0x200)) {
        std::cout << "ERROR: File is too large" << std::endl << "File must be of size " << MEMORY_SIZE - 0x200 << " or smaller" << std::endl << std::endl;
        std::cout << fsize;
        return false;
    }

    // Put the contents of the file in memory starting at address 0x200
    if (fread(&(memory[0x200]), 1, fsize, fp) != fsize) {
        std::cout << "ERROR: File reading error" << std::endl << std::endl;
        return false;
    }

    fclose(fp);
    return true;
}




// Does nothing
void Chip8::OP_NULL(){}

// Clear the display
void Chip8::OP_00E0() {
    drawFlag = true;

    for (int i = 0; i < VIDEO_WIDTH * VIDEO_HEIGHT; i++) {       
        video[i] = 0;
    }
}

// Return from a subroutine
void Chip8::OP_00EE() {
    sp--;
    pc = stack[sp];
}

// Jump to address nnn
void Chip8::OP_1nnn() {
    pc = opcode & 0x0FFF;
}

// Call subroutine at address nnn
void Chip8::OP_2nnn() {
    stack[sp] = pc;
    sp++;
    pc = opcode & 0x0FFF;
}

// Skip next instruction if Vx == kk (where kk is a byte)
void Chip8::OP_3xkk() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t kk = opcode & 0x00FF;

    if (V[x] == kk) {
        pc += 2;
    }
}

// Skip next instruction if Vx != kk (where kk is a byte)
void Chip8::OP_4xkk() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t kk = opcode & 0x00FF;

    if (V[x] != kk) {
        pc += 2;
    }
}

// Skip next instruction if Vx == Vy
void Chip8::OP_5xy0() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    if (V[x] == V[y]) {
        pc += 2;
    }
}

// Set Vx = kk (where kk is a byte)
void Chip8::OP_6xkk() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t kk = opcode & 0x00FF;

    V[x] = kk;
}

// Vx = Vx + kk (where kk is a byte)
void Chip8::OP_7xkk() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t kk = opcode & 0x00FF;

    V[x] += kk;
}

// Set Vx = Vy
void Chip8::OP_8xy0() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    V[x] = V[y];
}

// Set Vx = Vx OR Vy
// Also resets VF
void Chip8::OP_8xy1() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    V[x] = V[x] | V[y];
    V[0xF] = 0;
}

// Set Vx = Vx AND Vy
// Also resets VF
void Chip8::OP_8xy2() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    V[x] = V[x] & V[y];
    V[0xF] = 0;
}

// Set Vx = Vx XOR Vy
// Also resets VF
void Chip8::OP_8xy3() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    V[x] = V[x] ^ V[y];
    V[0xF] = 0;
}

// Set Vx = Vx + Vy 
// If the result is greater than 8 bits (>255) set VF = 1, otherwise VF = 0
// Only the 8 lowest bits are stored in Vx
// VF can also be either Vx or Vy
void Chip8::OP_8xy4() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    uint16_t sum = V[x] + V[y];
    V[x] = sum & 0xFF;

    if (sum > 255) {
        V[0xF] = 1;
    } else {
        V[0xF] = 0;
    }
}

// Set Vx = Vx - Vy
// If Vx > Vy, set VF to 1, otherwise VF = 0
// VF can also be either Vx or Vy
void Chip8::OP_8xy5() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    uint8_t val = V[x];

    V[x] = val - V[y];
    if (val >= V[y]) {
        V[0xF] = 1;
    } else {
        V[0xF] = 0;
    }
}

// If the least significant bit of Vx is 1, VF = 1, otherwise VF = 0
// Divide Vx by 2
// This variation of the opcode doesn't actually use Vy
// VF can also be either Vx or Vy
void Chip8::OP_8xy6() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t lsb = V[x] & 0x1;

    V[x] = V[x] >> 1;
    V[0xF] = lsb;
}

// Set Vx = Vy - Vx
// If Vy > Vx, set VF to 1, otherwise VF = 0
// VF can also be either Vx or Vy
void Chip8::OP_8xy7() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    uint8_t val = V[x];

    V[x] = V[y] - V[x];
    if (V[y] >= val) {
        V[0xF] = 1;
    } else {
        V[0xF] = 0;
    }
}

// If the most significant bit of Vx is 1, VF = 1, otherwise VF = 0
// Multiply Vx by 2
// This variation of the opcode doesn't actually use Vy
// VF can also be either Vx or Vy
void Chip8::OP_8xyE() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t msb = (V[x] & 0x80) >> 7;
    uint16_t val = V[x] << 1;

    V[x] = val & 0x01FE;
    V[0xF] = msb;
}

// Skip next instruction if Vx != Vy
void Chip8::OP_9xy0() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    if (V[x] != V[y])
        pc += 2;
}

// Set I = nnn
void Chip8::OP_Annn() {
    I = opcode & 0x0FFF;
}

// Jump to location nnn + V0
void Chip8::OP_Bnnn() {
    pc = (opcode & 0x0FFF) + V[0x0];
}

// Generate a random byte between 0 and 255 and AND it with kk
// Store the results in Vx
void Chip8::OP_Cxkk() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t kk = opcode & 0x00FF;
    int random = rand() % 256;

    V[x] = random & kk;
}

// Read n bytes of memory starting at the address stored in I
// Display these bytes as sprites on the screen at coordinates (Vx, Vy)
// Sprites are XOR'd onto the screen - if this causes sprites to be erased set VF = 1, otherwise VF = 0
// Sprites do not wrap around the edges of the screen - if they reach the edges they are clipped and cut off
// Coordinates wrap, so if x > 63 or y > 32, then x = x % 64 or y = y % 32, respectively
// A sprite is a group of bytes which are a binary representation of the desired picture - Chip-8 sprites may be up to 15 bytes, for a possible sprite size of 8x15
void Chip8::OP_Dxyn() {
    drawFlag = true;

    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    uint8_t n = (opcode & 0x000F);
    if (V[x] > 63)
        V[x] = V[x] % 64;
    if (V[y] > 31)
        V[y] = V[y] % 32;

    uint16_t vidx = (VIDEO_WIDTH * V[y]) + V[x];                // Index into the video buffer
    uint16_t start_vidx = vidx;                                 // Keeps track of what index the sprite will start being drawn from in each row
    uint16_t end_vidx = (VIDEO_WIDTH * V[y]) + VIDEO_WIDTH-1;   // Keeps track of what index the end of each row is at in case the sprite is clipped

    V[0xF] = 0;
    for (int i = 0; i < n; i++) {

        // Draw one row of bits
        uint8_t bit = 0x80;                                     // Used to check if a particular bit needs to be drawn, starts at 0b10000000 (0x80) and ends at 0b00000001 (0x01)
        for (int j = 0; j < 8; j++) {
            if (vidx > end_vidx) {                              // If vidx goes too far and clips past the edge of the screen
                break;                                          // Stop and go to the next row
            }

            if ((memory[I + i] & bit) && video[vidx]) {         // If a pixel was already here -AND- new pixel is being drawn here:
                V[0xF] = 1;                                     // Set VF = 1
                video[vidx] = 0;                                // 1 XOR 1 = 0
            } else if (memory[I + i] & bit) {                   // If no pixel was here and a new pixel must be drawn:
                video[vidx] = 1;                                // 0 XOR 1 = 1
            }

            bit = bit >> 1;       
            vidx++;                                         
        }
        
        // Time to go to the next row
        vidx = start_vidx;                                      // Reset vidx to its starting position to make it easier to calculate the start position for the next row

        vidx += VIDEO_WIDTH;                                    // Go to the next row in the video buffer
        start_vidx += VIDEO_WIDTH;                              // Set start_vidx to the start of the next row
        end_vidx += VIDEO_WIDTH;                                // Set end_vidx to the end of the next row       
        if (vidx >= VIDEO_WIDTH * VIDEO_HEIGHT) {               // If the sprite reaches the bottom of the screen, stop drawing
            break;
        }
    }
}

// Skip next instruction if a key with the value of Vx is pressed
void Chip8::OP_Ex9E() {
    uint8_t x = (opcode & 0x0F00) >> 8;

    if (keypad[V[x]])
        pc += 2;  
}

// Skip next instruction if a key with the value of Vx is NOT pressed 
void Chip8::OP_ExA1() {
    uint8_t x = (opcode & 0x0F00) >> 8;

    if (!keypad[V[x]])
        pc += 2;  
}

// Vx = delay_timer
void Chip8::OP_Fx07() {
    uint8_t x = (opcode & 0x0F00) >> 8;

    V[x] = delay_timer;
}

// Wait for a key press and store the value of the key in Vx
void Chip8::OP_Fx0A() {
    uint8_t x = (opcode & 0x0F00) >> 8;

    for (int i = 0; i < KEY_COUNT; i++) {
        if (keypad[i]) {
            V[x] = i;
            return;
        }  
    }

    pc -= 2;    // No key was pressed, so retry the opcode
}

// delay_timer = Vx
void Chip8::OP_Fx15() {
    uint8_t x = (opcode & 0x0F00) >> 8;

    delay_timer = V[x];
}

// sound_timer = Vx
void Chip8::OP_Fx18() {
    uint8_t x = (opcode & 0x0F00) >> 8;

    sound_timer = V[x];
}

// I = I + Vx
void Chip8::OP_Fx1E() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    
    I += V[x];
}

// Set I = location of the sprite for digit stored in Vx
void Chip8::OP_Fx29() {
    uint8_t x = (opcode & 0x0F00) >> 8;

    I = FONTSET_START_ADDRESS + (5 * V[x]);
}

// Take the decimal value of Vx, and place the hundreds digit in memory at location in I, the tens digit at location I+1, and the ones digit at location I+2
void Chip8::OP_Fx33() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t val = V[x];

    memory[I+2] = val % 10;
    val /= 10;

    memory[I+1] = val % 10;
    val /= 10;

    memory[I] = val % 10;
}

// Store registers V0 through Vx in memory starting at location I
// I is incremented in the COSMAC variant
void Chip8::OP_Fx55() {
    uint8_t x = (opcode & 0x0F00) >> 8;

    for (int i = 0; i <= x; i++) {
        memory[I] = V[i];
        I++;
    }
}

// Load registers V0 through Vx from memory starting at location I
// I is incremented in the COSMAC variant
void Chip8::OP_Fx65() {
    uint8_t x = (opcode & 0x0F00) >> 8;

    for (int i = 0; i <= x; i++) {
        V[i] = memory[I];
        I++;
    }
}




// Arguments (bitwise OR'd together):
// D_OP | D_PC | D_MEM_ALL | D_MEM_FONTS | D_MEM_ROM | D_SP | D_STACK | D_I | D_V | D_VID | D_KEYS | D_DT | D_ST | D_ALL
void Chip8::debug(uint16_t bitmask) {
    uint16_t d_op = bitmask & 0x1000;        
    uint16_t d_pc = bitmask & 0x800;        
    uint16_t d_mem_all = bitmask & 0x400;       
    uint16_t d_sp = bitmask & 0x200;       
    uint16_t d_stack = bitmask & 0x100;     
    uint16_t d_I = bitmask & 0x80;          
    uint16_t d_V = bitmask & 0x40;          
    uint16_t d_video = bitmask & 0x20;
    uint16_t d_keys = bitmask & 0x10;        
    uint16_t d_dt = bitmask & 0x8;          
    uint16_t d_st = bitmask & 0x4;          
    uint16_t d_mem_fonts = bitmask & 0x2;
    uint16_t d_mem_rom = bitmask & 0x1;


    if (d_dt) {
        std::cout << "Delay timer value: " << std::dec << delay_timer << std::endl << std::endl;
    }

    if (d_st) {
        std::cout << "Sound timer value: " << std::dec << sound_timer << std::endl << std::endl;
    }


    if (d_keys) {
        std::cout << "Keys currently pressed: " << std::endl;
        for (int i = 0; i < KEY_COUNT; i++) {
            if (keypad[i])
                std::cout << std::hex << i << std::endl;
        }
        std::cout << std::endl;
    }


    if (d_V) {
        for(int i = 0; i < REGISTER_COUNT; i++) {
            std::cout << "Value stored in V[0x" << std::hex << i << "]: " << std::hex << std::setw(2) << std::setfill('0') << (int)V[i] << std::endl;
        }
        std::cout << std::endl;
    }   


    if (d_stack) {
        std::cout << "SP points to stack level " << std::dec << sp << std::endl;
        for(int i = 0; i < STACK_SIZE; i++) {
            if (sp == i) {
                std::cout << "Stack level " << std::dec << i << " points to: 0x" << std::hex << std::setw(3) << std::setfill('0') << (int)stack[sp] << std::endl;
            } else {
                std::cout << "Stack level " << std::dec << i << " points to: 0x" << std::hex << std::setw(3) << std::setfill('0') << (int)stack[sp] << "\t<< SP points here" << std::endl;
            }
        }
        std::cout << std::endl;
    } else if (d_sp) {
        std::cout << "SP points to stack level " << std::dec << sp << std::endl << std::endl;
    }


    if (d_op) 
        std::cout << "Latest opcode executed: " << std::hex << std::setw(4) << std::setfill('0') << (int)opcode << std::endl << std::endl;


    if (d_mem_all) {
        for (int i = 0; i < MEMORY_SIZE; i++) {
            if (memory[i]) {
                std::cout << "Value at memory index " << std::dec << (int)i << " (memory address 0x" << std::hex << std::setw(3) << std::setfill('0') << (int)i << "): 0x" 
                << std::hex << std::setw(2) << std::setfill('0') << (int)memory[i];
                if (pc == i)
                    std::cout << "\t<< PC points here";
                if (I == i)
                    std::cout << "\t<< I points here";
                std::cout << std::endl;
            }
        }
        std::cout << std::endl;
    } else if (d_mem_fonts) {
        for (int i = 0; i < 0x1FF; i++) {
            if (memory[i]) {
                    std::cout << "Value at memory index " << std::dec << (int)i << " (memory address 0x" << std::hex << std::setw(3) << std::setfill('0') << (int)i << "): 0x" 
                    << std::hex << std::setw(2) << std::setfill('0') << (int)memory[i];
                if (pc == i)
                    std::cout << "\t<< PC points here";
                if (I == i)
                    std::cout << "\t<< I points here";
                std::cout << std::endl;
            }
        }
        std::cout << std::endl;
    } else if (d_mem_rom) {
        for (int i = 0x200; i < MEMORY_SIZE; i++) {
            if (memory[i]) {
                std::cout << "Value at memory index " << std::dec << (int)i << " (memory address 0x" << std::hex << std::setw(3) << std::setfill('0') << (int)i << "): 0x" 
                << std::hex << std::setw(2) << std::setfill('0') << (int)memory[i];
                if (pc == i)
                    std::cout << "\t<< PC points here";
                if (I == i)
                    std::cout << "\t<< I points here";
                std::cout << std::endl;
            }
        }
        std::cout << std::endl;
        
    } else if (d_pc && d_I) {
        std::cout << "PC points to: 0x" << std::hex << std::setw(3) << std::setfill('0') << (int)pc << std::endl;
        std::cout << "I points to: 0x" << std::hex << std::setw(3) << std::setfill('0') << (int)I << std::endl;
    } else if (d_pc) {
        std::cout << "PC points to: 0x" << std::hex << std::setw(3) << std::setfill('0') << (int)pc << std::endl << std::endl;
    } else if (d_I) {
        std::cout << "I points to: 0x" << std::hex << std::setw(3) << std::setfill('0') << (int)I << std::endl << std::endl;
    }    


    if (d_video) {
        std::cout << "VIDEO: " << std::endl;
        for (int i = 0; i < VIDEO_WIDTH * VIDEO_HEIGHT; i++) {
            if (i % VIDEO_WIDTH == 0) 
                std::cout << std::endl;

            if (video[i]) {
                std::cout << "1";
            } else {
                std::cout << "0";
            }
        }
        std::cout << std::endl;
    }
    
    std::cout << "------------------------------------------------------------------------------------------------" << std::endl << std::endl;
}