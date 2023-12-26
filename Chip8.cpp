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
    void (Chip8::*OpcodeTable[0xF + 1])() = {
        &Chip8::GetTable0,
        &Chip8::OP_1nnn,
        &Chip8::OP_2nnn,
        &Chip8::OP_3xkk,
        &Chip8::OP_4xkk,
        &Chip8::OP_5xy0,
        &Chip8::OP_6xkk,
        &Chip8::OP_7xkk,
        &Chip8::GetTable8,
        &Chip8::OP_9xy0,
        &Chip8::OP_Annn,
        &Chip8::OP_Bnnn,
        &Chip8::OP_Cxkk,
        &Chip8::OP_Dxyn,
        &Chip8::GetTableE,
        &Chip8::GetTableF
    };


    // Initialize opcode tables 0, 8, and E with all NULL values
    for (int i = 0; i < 0xE + 1; i++) {
        OpcodeTable_0[i] = OpcodeTable_8[i] = OpcodeTable_E[i] = Chip8::OP_NULL;
    }

    // Fill the proper indices with their opcodes
    OpcodeTable_0[0x0] = Chip8::OP_00E0;
    OpcodeTable_0[0xE] = Chip8::OP_00EE;

    OpcodeTable_8[0x0] = Chip8::OP_8xy0;
    OpcodeTable_8[0x1] = Chip8::OP_8xy1;
    OpcodeTable_8[0x2] = Chip8::OP_8xy2;
    OpcodeTable_8[0x3] = Chip8::OP_8xy3;
    OpcodeTable_8[0x4] = Chip8::OP_8xy4;
    OpcodeTable_8[0x5] = Chip8::OP_8xy5;
    OpcodeTable_8[0x6] = Chip8::OP_8xy6;
    OpcodeTable_8[0x7] = Chip8::OP_8xy7;
    OpcodeTable_8[0xE] = Chip8::OP_8xyE;

    OpcodeTable_E[0xE] = Chip8::OP_Ex9E;
    OpcodeTable_E[0x1] = Chip8::OP_ExA1;


    // Initialize opcode table F with all NULL values
    for (int i = 0; i < 0x65 + 1; i++) {
        OpcodeTable_F[i] = Chip8::OP_NULL;
    }

    // Fill the proper indices with their opcodes
    OpcodeTable_F[0x07] = Chip8::OP_Fx07;
    OpcodeTable_F[0x0A] = Chip8::OP_Fx0A;
    OpcodeTable_F[0x15] = Chip8::OP_Fx15;
    OpcodeTable_F[0x18] = Chip8::OP_Fx18;
    OpcodeTable_F[0x1E] = Chip8::OP_Fx1E;
    OpcodeTable_F[0x29] = Chip8::OP_Fx29;
    OpcodeTable_F[0x33] = Chip8::OP_Fx33;
    OpcodeTable_F[0x55] = Chip8::OP_Fx55;
    OpcodeTable_F[0x65] = Chip8::OP_Fx65;

    // Initialize seed for random number generator
    srand(time(NULL));
}




// Index into OpcodeTable_0 based on the last hex digit of the opcode
void Chip8::GetTable0() {
    int index = opcode & 0x000F;

    // Dereference OpcodeTable_0 and call the function at the index
    ((*this).*(OpcodeTable_0[index]))();
}


// Index into OpcodeTable_8 based on the last hex digit of the opcode
void Chip8::GetTable8() {
    int index = opcode & 0x000F;

    // Dereference OpcodeTable_8 and call the function at the index
    ((*this).*(OpcodeTable_8[index]))();
}


// Index into OpcodeTable_E based on the last hex digit of the opcode
void Chip8::GetTableE() {
    int index = opcode & 0x000F;

    // Dereference OpcodeTable_E and call the function at the index
    ((*this).*(OpcodeTable_E[index]))();
}


// Index into OpcodeTable_F based on the last two hex digits of the opcode
void Chip8::GetTableF() {
    int index = opcode & 0x00FF;

    // Dereference OpcodeTable_0 and call the function at the index
    ((*this).*(OpcodeTable_0[index]))();
}




// Every cycle the CPU will fetch the opcode, decode the opcode, execute the opcode, and update timers
void Chip8::emulateCycle() {
    
    // Fetch Opcode
    opcode = (memory[pc] << 8) | memory[pc+1];      // Each opcode is 2 bytes long
                                                    // Need to merge the two halves in memory

    pc += 2;

    // Decode and Execute Opcode
    ((*this).*(OpcodeTable[(opcode & 0xF000) >> 12]))();


    // Update timers
    if (delay_timer) 
        delay_timer--;
    
    if (sound_timer) {
        // PLAY BEEP
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




void Chip8::debug() {
    std::cout << "Delay timer value: " << std::dec << (int)delay_timer << std::endl;
    std::cout << "Sound timer value: " << std::dec << (int)sound_timer << std::endl << std::endl;

    std::cout << "Currently stored opcode: 0x" << std::setw(4) << std::setfill('0') << std::hex << opcode << std::endl << std::endl;

    std::cout << "SP: " << std::dec << (int)sp << std::endl;
    for (int i = 0; i < STACK_SIZE; i++) {
        std::cout << "Address stored in stack level " << std::dec << i << ": 0x" << std::hex << (int)stack[i];
        if (sp == i) {
            std::cout << " << SP points here";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

    for (int i = 0; i < REGISTER_COUNT; i++) {
        std::cout << "Data stored in V[" << std::hex << i << "]: 0x" << std::hex << (int)V[i] << std::endl;
    }
    std::cout << std::endl;

    std::cout << "Address stored in I: 0x" << std::hex << I << std::endl;
    std::cout << "Address stored in PC: 0x" << std::hex << pc << std::endl;
    for (int i = 0; i < MEMORY_SIZE; i++) {
        if (i == 0x200) 
            std::cout << std::endl;

        if (i == pc) {
            std::cout << "Data stored in memory at index " << std::dec << i << " (memory address 0x" << std::setw(3) << std::setfill('0') << std::hex << i << "): 0x" << (int)memory[i] << "\t<< PC points here" << std::endl;
        } else if (i == I) {
            std::cout << "Data stored in memory at index " << std::dec << i << " (memory address 0x" << std::setw(3) << std::setfill('0') << std::hex << i << "): 0x" << (int)memory[i] << "\t<< Address stored in I points here" << std::endl;
        } else if (memory[i]) { 
            std::cout << "Data stored in memory at index " << std::dec << i << " (memory address 0x" << std::setw(3) << std::setfill('0') << std::hex << i << "): 0x" << (int)memory[i] << std::endl;
        }
    }
    std::cout << std::endl;

    std::cout << "Keys pressed: " << std::endl;
    for (int i = 0; i < KEY_COUNT; i++) {
        if (keypad[i])
            std::cout << std::hex << i << std::endl;
    }
    std::cout << std::endl;

    std::cout << "VIDEO: ";
    for (int i = 0; i < VIDEO_WIDTH * VIDEO_HEIGHT; i++) {
        if (i % VIDEO_WIDTH == 0)
            std::cout << std::endl;
        
        if (video[i]) {
            std::cout << "1";
        } else {
            std::cout << "*";
        }
    }
    std::cout << std::endl << "------------------------------------------------------------------------------------------------" <<  std::endl << std::endl;
}




// Does nothing
void Chip8::OP_NULL(){}

// Clear the display
void Chip8::OP_00E0() {
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
void Chip8::OP_8xy1() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    V[x] = V[x] | V[y];
}

// Set Vx = Vx AND Vy
void Chip8::OP_8xy2() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    V[x] = V[x] & V[y];
}

// Set Vx = Vx XOR Vy
void Chip8::OP_8xy3() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    V[x] = V[x] ^ V[y];
}

// Set Vx = Vx + Vy 
// If the result is greater than 8 bits (>255) set VF = 1, otherwise VF = 0
// Only the 8 lowest bits are stored in Vx
void Chip8::OP_8xy4() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    V[0xF] = 0;

    uint16_t sum = V[x] + V[y];

    if (sum > 255)
        V[0xF] = 1;

    V[x] = sum & 0xFF;
}

// Set Vx = Vx - Vy
// If Vx > Vy, set VF to 1, otherwise VF = 0
void Chip8::OP_8xy5() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    V[0xF] = 0;

    if (V[x] > V[y])
        V[0xF] = 1;

    V[x] -= V[y];
}

// If the least significant bit of Vx is 1, VF = 1, otherwise VF = 0
// Divide Vx by 2
void Chip8::OP_8xy6() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    V[0xF] = V[x] & 0x1;    // LSB gets saved in VF

    V[x] = V[x] >> 1;
}

// Set Vx = Vy - Vx
// If Vy > Vx, set VF to 1, otherwise VF = 0
void Chip8::OP_8xy7() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    V[0xF] = 0;

    if (V[y] > V[x])
        V[0xF] = 1;

    V[x] = V[y] - V[x];
}

// If the most significant bit of Vx is 1, VF = 1, otherwise VF = 0
// Multiply Vx by 2
void Chip8::OP_8xyE() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;

    V[0xF] = V[x] & 0x80;    // MSB gets saved in VF

    V[x] = V[x] << 1;
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
// Sprites wrap around the edges of the screen
// A sprite is a group of bytes which are a binary representation of the desired picture - Chip-8 sprites may be up to 15 bytes, for a possible sprite size of 8x15
void Chip8::OP_Dxyn() {
    uint8_t x = (opcode & 0x0F00) >> 8;
    uint8_t y = (opcode & 0x00F0) >> 4;
    uint8_t n = (opcode & 0x000F);
    uint16_t vidx = (VIDEO_WIDTH * V[y]) + V[x];     // Index into the video buffer
    uint16_t start_vidx = vidx;                      // Keeps track of what index the sprite will start being drawn from in each row
    uint16_t w_vidx = VIDEO_WIDTH * V[y];            // If the sprite needs to be wrapped around the edge of the screen then the wrapped index will be stored here

    V[0xF] = 0;
    for (int i = 0; i < n; i++) {

        // Draw one row of bits
        uint8_t bit = 0x80;                                     // Used to check if a particular bit needs to be drawn, starts at 0b10000000 (0x80) and ends at 0b00000001 (0x01)
        for (int j = 0; j < 8; j++) {
            if (vidx >= w_vidx + VIDEO_WIDTH) {                 // If vidx goes too far and instead must be wrapped around the screen
                vidx = w_vidx;                                  // Store the wrapped index in vidx
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
        w_vidx += VIDEO_WIDTH;                                  // Go to next row for the wrapped index as well
        if (vidx >= VIDEO_WIDTH * VIDEO_HEIGHT) {               // Wrap around to the top of the screen if required to do so
            vidx = V[x];
            w_vidx = 0;
        }
        start_vidx = vidx;
    }
}

// Skip next instruction if a key with the value of Vx is pressed
void Chip8::OP_Ex9E() {
    uint8_t x = (opcode & 0x0F00) >> 8;

    if (keypad[V[x]])
        pc =+ 2;  
}

// Skip next instruction if a key with the value of Vx is NOT pressed 
void Chip8::OP_ExA1() {
    uint8_t x = (opcode & 0x0F00) >> 8;

    if (!keypad[V[x]])
        pc =+ 2;  
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
            break;
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
// 0-F represented as 0-15 in register
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
void Chip8::OP_Fx55() {
    uint8_t x = (opcode & 0x0F00) >> 8;

    for (int i = 0; i <= x; i++) {
        memory[I + i] = V[i];
    }
}

// Read registers V0 through Vx from memory starting at location I
void Chip8::OP_Fx65() {
    uint8_t x = (opcode & 0x0F00) >> 8;

    for (int i = 0; i <= x; i++) {
        V[i] = memory[I + i];
    }
}