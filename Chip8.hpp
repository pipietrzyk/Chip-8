#pragma once

#include <cstdint>

// COSMAC VIP variant

const unsigned int KEY_COUNT = 16;
const unsigned int VIDEO_WIDTH = 64;
const unsigned int VIDEO_HEIGHT = 32;
const unsigned int REGISTER_COUNT = 16;
const unsigned int MEMORY_SIZE = 4096;
const unsigned int STACK_SIZE = 16;

// Flags for the debug() function -- OR'd together
const uint16_t D_OP = 0b1000000000000;                      // Show opcode
const uint16_t D_PC = 0b0100000000000;                      // Show PC
const uint16_t D_MEM_ALL = 0b0010000000000;                 // Show memory from 0x000 to 0xFFF -- also shows I and PC as well
const uint16_t D_MEM_FONTS = 0b0000000000010;               // Show memory from 0x000 to 0x1FF -- also shows I and PC as well
const uint16_t D_MEM_ROM = 0b0000000000001;                 // Show memory from 0x200 to 0xFFF -- also shows I and PC as well
const uint16_t D_SP = 0b0001000000000;                      // Show SP
const uint16_t D_STACK = 0b0000100000000;                   // Show stack (and SP)
const uint16_t D_I = 0b0000010000000;                       // Show index register
const uint16_t D_V = 0b0000001000000;                       // Show registers
const uint16_t D_VID = 0b0000000100000;                     // Show a representation of the video
const uint16_t D_KEYS = 0b0000000010000;                    // Show which keys were pressed
const uint16_t D_DT = 0b0000000001000;                      // Show delay timer
const uint16_t D_ST = 0b0000000000100;                      // Show sound timer
const uint16_t D_ALL = 0b1111111111111;                     // Show everything


class Chip8 {
    public:
        Chip8();

        void emulateCycle();                                // Emulates one cycle of the CPU (60 cycles per second)
		bool loadROM(const char * filename);                // Loads a ROM into the program memory (starting at 0x200)
        void updateTimers();                                // Updates the delay timer and sound timer and plays a tone while the sound timer is > 0

        void debug(uint16_t bitmask);                       // Prints out the current states of class variables
                                                            // A bitmask is used to select which variables should be output

        uint8_t keypad[KEY_COUNT]{};                        // Used for keypad input
        uint32_t video[VIDEO_WIDTH * VIDEO_HEIGHT]{};       // Used to represent the display
                                                            // Each pixel is either ON or OFF

        bool drawFlag;                                      // A flag used to determine whether the emulator should draw to the display or not
                                                            // Set by OP_00E0() and OP_DXYN()
                                                            // Only needs to draw if something new should be drawn to the screen

    private:

        uint16_t pc;                                        // Program counter

        uint8_t sp;                                         // Stack pointer
        uint16_t I;                                         // Register which stores memory addresses for use in operations
        uint16_t opcode;                                    // Stores the current opcode

        uint8_t delay_timer;                                // Used for timing
                                                            // When non-zero, decrements at a rate of 60hz
        uint8_t sound_timer;                                // Used for sound
                                                            // When non-zero, decrements at a rate of 60hz while playing a tone

        uint8_t V[REGISTER_COUNT];                          // 16 registers (V0-VF)
                                                            // VF should not be used by programs, it is used as a flag by some instructions
        uint8_t memory[MEMORY_SIZE];                        // 4KB of memory
        uint16_t stack[STACK_SIZE];                         // Stack with 16 levels


        void initialize();                                  // Initialize registers and memory


        void getTable0();                                   // Indexes into OpcodeTable_0
        void getTable8();                                   // Indexes into OpcodeTable_8
        void getTableE();                                   // Indexes into OpcodeTable_E   
        void getTableF();                                   // Indexes into OpcodeTable_F


        // Tables of pointers to opcodes
        typedef void (Chip8::*opTable)();
        opTable OpcodeTable[0xF + 1];
        opTable OpcodeTable_0[0xE + 1];
        opTable OpcodeTable_8[0xE + 1];
        opTable OpcodeTable_E[0xE + 1];
        opTable OpcodeTable_F[0x65 + 1];
    

        // Functions which execute opcodes
        void OP_NULL();                     // Does nothing
                                   
        void OP_00E0();                     // Clear screen
        void OP_00EE();                     // Return from subroutine              
    
        void OP_1nnn();                     // Jump to address nnn

        void OP_2nnn();                     // Calls subroutine at address nnn

        void OP_3xkk();                     // Skip if equal Vx, byte

        void OP_4xkk();                     // Skip if not equal Vx, byte

        void OP_5xy0();                     // Skip if equal Vx, Vy

        void OP_6xkk();                     // LD Vx, byte

        void OP_7xkk();                     // ADD Vx, byte

        void OP_8xy0();                     // LD Vx, Vy
        void OP_8xy1();                     // OR Vx, Vy     
        void OP_8xy2();                     // AND Vx, Vy
        void OP_8xy3();                     // XOR Vx, Vy
        void OP_8xy4();                     // ADD Vx, Vy
        void OP_8xy5();                     // SUB Vx, Vy
        void OP_8xy6();                     // Shift right Vx
        void OP_8xy7();                     // Vx = Vy - Vx
        void OP_8xyE();                     // Shift left Vx

        void OP_9xy0();                     // Skip if not equal Vx, Vy

        void OP_Annn();                     // LD I, addr

        void OP_Bnnn();                     // Jump to address nnn + V0

        void OP_Cxkk();                     // Vx = rand() & kk

        void OP_Dxyn();                     // DRW Vx, Vy, nibble

        void OP_Ex9E();                     // Skip if key pressed Vx
        void OP_ExA1();                     // Skip if key not pressed Vx

        void OP_Fx07();                     // LD Vx, DT
        void OP_Fx0A();                     // Wait for key press and store it in Vx
        void OP_Fx15();                     // LD DT, Vx
        void OP_Fx18();                     // LD ST, Vx
        void OP_Fx1E();                     // ADD I, Vx
        void OP_Fx29();                     // Sets I to the location of the sprite for the character in Vx
        void OP_Fx33();                     // Store decimal digits of Vx in I, I+1, I+2
        void OP_Fx55();                     // Store V0 - Vx starting at I
        void OP_Fx65();                     // Load V0 - Vx starting at I
};