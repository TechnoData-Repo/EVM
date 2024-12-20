/*
* AUTHOR: ERICK AFFONSO (2024)
*
* FREE AND OPEN-SOURCE SOFTWARE
*
* SOFTWARE LICENSE: GNU GPLv3
*/

#include <iostream>
#include <chrono>
#include <thread>
#include <fstream>
#include <vector>
#include <definitions.h>
#include <SDL.h>
#include <via6522.h>
#include <keyboard.h>
#include <ssd.h>
#include <mos65c02.h>
#include <sas.h>

// Keyboard Layout to be used
const uchar KBD_LAYOUT = US;

extern byte Memory[0x10000];
/* Layout:
	* $0000-$3FCF (RAM 1)
	* $3FE0-$3FEF (VIA 3)
	* $3FE0-$3FEF (VIA 2)
	* $3FF0-$3FFF (VIA 1)
	* $4000-$7FFF (RAM 2 -- VIDEO RESERVED SPACE)
	* $8000-$BFFF (RAM 3)
	* $C000-$FFFF (ROM)
*/

// Simple and Square Video Control Unit
SASVCU vcu;
const int SCREEN_WIDTH = 768;
const int SCREEN_HEIGHT = 768;
int FPS;

// VIAs
VIA6522 viaOne; // VIA 6522 | 1 ($3FF0-$3FFF)
VIA6522 viaTwo; // VIA 6522 | 2 ($3FE0-$3FEF)
VIA6522 viaThree; // VIA 6522 | 2 ($3FD0-$3FDF)

// SSD
SSD ssd;
void SSD_RW() {
	ssd.executeInstruction();
}

// CPU
MOS65C02 cpu;
// CPU pins
bool RES = false; // Reset Pin (Active-low)
bool RDY = true; // Ready Pin (Active-high)
bool NMI = true; // Non-Maskable Interrupt (Active-low)
bool IRQ = true; // Interrupt Request Queue (Active-low)

// --- Emulator-specific variables ---
bool PowerON = true;
bool verbose = false; // Verbosity
bool clkTest = false; // Clock Test
const int ROM_SIZE = 0x4000; // ROM size
bool SDLStatus = true; // SDL Running
const char* EmulatorSDLWindowName = "EVM (Erick's Virtual Machine)";
const char* Version = "alpha";

// Draws the Pixels on the Window, and adjusts the pixel size according to window size
void Draw(SDL_Renderer* renderer, int scale) {
	SDL_Rect rect;
	rect.x = vcu.HR * scale;
	rect.y = vcu.VR * scale;
	rect.h = scale;
	rect.w = scale;
	SDL_SetRenderDrawColor(renderer, vcu.RGB[0], vcu.RGB[1], vcu.RGB[2], SDL_ALPHA_OPAQUE);
	SDL_RenderFillRect(renderer, &rect);
}

// Video Control Unit
void VCU() {
	// Initializing Stuff
	vcu.reset();
	int scale = 0; // <-- Pixel scale
	if (vcu.CMR == 1) {
		scale = SCREEN_WIDTH / 256;
	}
	else {
		scale = SCREEN_WIDTH / 128;
	}

	// Initializing SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		std::cerr << "ERROR: SDL could not be initialized! SDL_Error: " << SDL_GetError() << std::endl;
		exit(-1);
	}

	// Window
	SDL_Window* window = SDL_CreateWindow(EmulatorSDLWindowName, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
	if (window == nullptr)
	{
		std::cerr << "ERROR: Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		SDL_Quit();
		exit(-1);
	}

	// Renderer
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
	if (renderer == nullptr)
	{
		std::cerr << "ERROR: Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
		SDL_DestroyWindow(window);
		SDL_Quit();
		exit(-1);
	}

	// Main loop flag
	bool quit = false;
	SDL_Event e;

	// Initializing Screen
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
	SDL_RenderClear(renderer);

	printf(" --- VCU Running\n");

	const int REPORT_BUFFER_SIZE = 8; // Size of the Keyboard Repport Buffer
	byte KeyboardReport[REPORT_BUFFER_SIZE][8]; // Keyboard Reports list
	bool ReportPacketStatus[REPORT_BUFFER_SIZE] = { true,true,true,true,true,true,true,true }; // Status of each Report Packet | false: not sent; true: sent;
	uchar KeysPressed = 2; // Number of Keys currently pressed (excluding modifier keys)
	uchar ReportPacket = 0; // Index of the Next Keyboard Report to be sent
	uchar ReportsStorageIndex = 0; // Index of the next Keyboard Report to be stored
	uchar ByteCounter = 0; // Bytes of the Keyboard Report that were already sent

	// Initializing KeyboardReport
	for (uchar i = 0; i < REPORT_BUFFER_SIZE; i++) {
		for (uchar j = 0; j < 8; j++) {
			KeyboardReport[i][j] = 0;
		}
	}

	auto start = std::chrono::high_resolution_clock::now(); // <- Used to get number of frames
	// SDL Main loop
	while (!quit)
	{
		// Handle events on the queue
		while (SDL_PollEvent(&e) != 0)
		{
			uchar Key = 0;
			// User requests quit
			if (e.type == SDL_QUIT)
			{
				quit = true;
			}
			else if (e.type == SDL_KEYDOWN) {
				Key = TranslateKey(e.key.keysym.sym, KBD_LAYOUT);
				if (verbose) {
					printf("Pressed. ASCII Key Code: %02x\n", e.key.keysym.sym);
				}
				if (ReportPacketStatus[ReportsStorageIndex] == true) {
					if (ReportsStorageIndex > 0) {
						KeyboardReport[ReportsStorageIndex][0] = KeyboardReport[ReportsStorageIndex - 1][0];
						KeyboardReport[ReportsStorageIndex][2] = KeyboardReport[ReportsStorageIndex - 1][2];
						KeyboardReport[ReportsStorageIndex][3] = KeyboardReport[ReportsStorageIndex - 1][3];
						KeyboardReport[ReportsStorageIndex][4] = KeyboardReport[ReportsStorageIndex - 1][4];
						KeyboardReport[ReportsStorageIndex][5] = KeyboardReport[ReportsStorageIndex - 1][5];
						KeyboardReport[ReportsStorageIndex][6] = KeyboardReport[ReportsStorageIndex - 1][6];
						KeyboardReport[ReportsStorageIndex][7] = KeyboardReport[ReportsStorageIndex - 1][7];
					}
					else {
						KeyboardReport[ReportsStorageIndex][0] = KeyboardReport[REPORT_BUFFER_SIZE - 1][0];
						KeyboardReport[ReportsStorageIndex][2] = KeyboardReport[REPORT_BUFFER_SIZE - 1][2];
						KeyboardReport[ReportsStorageIndex][3] = KeyboardReport[REPORT_BUFFER_SIZE - 1][3];
						KeyboardReport[ReportsStorageIndex][4] = KeyboardReport[REPORT_BUFFER_SIZE - 1][4];
						KeyboardReport[ReportsStorageIndex][5] = KeyboardReport[REPORT_BUFFER_SIZE - 1][5];
						KeyboardReport[ReportsStorageIndex][6] = KeyboardReport[REPORT_BUFFER_SIZE - 1][6];
						KeyboardReport[ReportsStorageIndex][7] = KeyboardReport[REPORT_BUFFER_SIZE - 1][7];
					}

					switch (e.key.keysym.sym) {
						case 0x400000E0: // Left CTRL
							KeyboardReport[ReportsStorageIndex][0] = KeyboardReport[ReportsStorageIndex][0] | 0b00000001;
							break;
						case 0x400000E1: // Left Shift
							KeyboardReport[ReportsStorageIndex][0] = KeyboardReport[ReportsStorageIndex][0] | 0b00000010;
							break;
						case 0x400000E2: // Left Alt
							KeyboardReport[ReportsStorageIndex][0] = KeyboardReport[ReportsStorageIndex][0] | 0b00000100;
							break;
						case 0x400000E4: // Right CTRL
							KeyboardReport[ReportsStorageIndex][0] = KeyboardReport[ReportsStorageIndex][0] | 0b00010000;
							break;
						case 0x400000E5: // Right Shift
							KeyboardReport[ReportsStorageIndex][0] = KeyboardReport[ReportsStorageIndex][0] | 0b00100000;
							break;
						case 0x400000E6: // Right Alt
							KeyboardReport[ReportsStorageIndex][0] = KeyboardReport[ReportsStorageIndex][0] | 0b01000000;
							break;
					}
					bool AlreadyReported = false;
					for (uchar i = 2; i < 8; i++) {
						if (KeyboardReport[ReportsStorageIndex][i] == Key) {
							AlreadyReported = true;
							break;
						}
					}
					if (AlreadyReported == false) {
						if (KeysPressed < 8) {
							KeyboardReport[ReportsStorageIndex][KeysPressed] = Key;
							KeysPressed++;
						}
						ReportPacketStatus[ReportsStorageIndex] = false;
						if (ReportsStorageIndex < (REPORT_BUFFER_SIZE - 1)) {
							ReportsStorageIndex++;
						}
						else {
							ReportsStorageIndex = 0;
						}
					}
					else {
						ReportPacketStatus[ReportsStorageIndex] = false;
						if (ReportsStorageIndex < (REPORT_BUFFER_SIZE - 1)) {
							ReportsStorageIndex++;
						}
						else {
							ReportsStorageIndex = 0;
						}
					}
				}
			}
			else if (e.type == SDL_KEYUP) {
				Key = TranslateKey(e.key.keysym.sym, KBD_LAYOUT);
				if (verbose) {
					printf("Released. ASCII Key Code: %02x\n", e.key.keysym.sym);
				}
				if (ReportPacketStatus[ReportsStorageIndex] == true) {
					if (ReportsStorageIndex > 0) {
						KeyboardReport[ReportsStorageIndex][0] = KeyboardReport[ReportsStorageIndex - 1][0];
						KeyboardReport[ReportsStorageIndex][2] = KeyboardReport[ReportsStorageIndex - 1][2];
						KeyboardReport[ReportsStorageIndex][3] = KeyboardReport[ReportsStorageIndex - 1][3];
						KeyboardReport[ReportsStorageIndex][4] = KeyboardReport[ReportsStorageIndex - 1][4];
						KeyboardReport[ReportsStorageIndex][5] = KeyboardReport[ReportsStorageIndex - 1][5];
						KeyboardReport[ReportsStorageIndex][6] = KeyboardReport[ReportsStorageIndex - 1][6];
						KeyboardReport[ReportsStorageIndex][7] = KeyboardReport[ReportsStorageIndex - 1][7];
					}
					else {
						KeyboardReport[ReportsStorageIndex][0] = KeyboardReport[REPORT_BUFFER_SIZE - 1][0];
						KeyboardReport[ReportsStorageIndex][2] = KeyboardReport[REPORT_BUFFER_SIZE - 1][2];
						KeyboardReport[ReportsStorageIndex][3] = KeyboardReport[REPORT_BUFFER_SIZE - 1][3];
						KeyboardReport[ReportsStorageIndex][4] = KeyboardReport[REPORT_BUFFER_SIZE - 1][4];
						KeyboardReport[ReportsStorageIndex][5] = KeyboardReport[REPORT_BUFFER_SIZE - 1][5];
						KeyboardReport[ReportsStorageIndex][6] = KeyboardReport[REPORT_BUFFER_SIZE - 1][6];
						KeyboardReport[ReportsStorageIndex][7] = KeyboardReport[REPORT_BUFFER_SIZE - 1][7];
					}

					switch (e.key.keysym.sym) {
						case 0x400000E0: // Left CTRL
							KeyboardReport[ReportsStorageIndex][0] = KeyboardReport[ReportsStorageIndex][0] & 0b11111110;
							break;
						case 0x400000E1: // Left Shift
							KeyboardReport[ReportsStorageIndex][0] = KeyboardReport[ReportsStorageIndex][0] & 0b11111101;
							break;
						case 0x400000E2: // Left Alt
							KeyboardReport[ReportsStorageIndex][0] = KeyboardReport[ReportsStorageIndex][0] & 0b11111011;
							break;
						case 0x400000E4: // Right CTRL
							KeyboardReport[ReportsStorageIndex][0] = KeyboardReport[ReportsStorageIndex][0] & 0b11101111;
							break;
						case 0x400000E5: // Right Shift
							KeyboardReport[ReportsStorageIndex][0] = KeyboardReport[ReportsStorageIndex][0] & 0b11011111;
							break;
						case 0x400000E6: // Right Alt
							KeyboardReport[ReportsStorageIndex][0] = KeyboardReport[ReportsStorageIndex][0] & 0b10111111;
							break;
					}
					for (uchar i = 2; i < 8; i++) {
						if (KeyboardReport[ReportsStorageIndex][i] == Key) {
							while (i != 7) {
								KeyboardReport[ReportsStorageIndex][i] = KeyboardReport[ReportsStorageIndex][i + 1];
								i++;
							}
							KeyboardReport[ReportsStorageIndex][i] = 0x00;
							if (KeysPressed > 2) {
								KeysPressed--;
							}
							ReportPacketStatus[ReportsStorageIndex] = false;
							if (ReportsStorageIndex < (REPORT_BUFFER_SIZE - 1)) {
								ReportsStorageIndex++;
							}
							else {
								ReportsStorageIndex = 0;
							}
							break;
						}
					}
				}
			}
		}

		// Send Keyboard Report to the CPU (8-bits at a time)
		if (IRQ == true && ReportPacketStatus[ReportPacket] == false) {
			viaOne.PA = KeyboardReport[ReportPacket][ByteCounter] & (~(viaOne.DDRA));
			viaOne.CA1 = true; // Trigger CA1
			viaOne.setInterrupt(); // Set up the Interrupt for the CPU
			IRQ = viaOne.checkInterrupt(); // Trigger the Interrupt
			if (ByteCounter == 7) {
				if (verbose) {
					printf("Keyboard Report Packet Sent: %02x%02x%02x%02x%02x%02x%02x%02x\n", KeyboardReport[ReportPacket][7], KeyboardReport[ReportPacket][6],
						KeyboardReport[ReportPacket][5], KeyboardReport[ReportPacket][4], KeyboardReport[ReportPacket][3], KeyboardReport[ReportPacket][2],
						KeyboardReport[ReportPacket][1], KeyboardReport[ReportPacket][0]);
				}
				ReportPacketStatus[ReportPacket] = true;
				if (ReportPacket < (REPORT_BUFFER_SIZE - 1)) {
					ReportPacket++;
				}
				else {
					ReportPacket = 0;
				}
				ByteCounter = 0;
			}
			else {
				ByteCounter++;
			}
		}

		// VCU Subroutines
		vcu.FetchPixelData();
		vcu.TranslatePixel();
		Draw(renderer, scale);
		vcu.IncVideoRegs();

		// Reset VCU
		if (!RES || (viaOne.PB & 0b00000001)) {
			vcu.reset();
			if (vcu.CMR == 1) {
				scale = SCREEN_WIDTH / 256;
			}
			else {
				scale = SCREEN_WIDTH / 128;
			}
			viaOne.PB = 0;
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
			SDL_RenderClear(renderer);
		}

		if (vcu.HR == 0) {
			if (vcu.VR == 0) {
				SDL_RenderPresent(renderer);

				if (clkTest) {
					auto end = std::chrono::high_resolution_clock::now();
					FPS++;
					if (std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() >= 1000) {
						std::cout << "VCU FPS: " << FPS << std::endl;
						FPS = 0;
						start = std::chrono::high_resolution_clock::now();
					}
				}
			}
		}
	}

	// Kill SDL instance
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
	SDLStatus = false;
}

// Reset RAM
void RAMReset() {
	for (unsigned int i = 0; i < 0x10000; i++) {
		if (i < (0xFFFF - ROM_SIZE)) {
			Memory[i] = 0x00;
		}
	}
}

// Central Processing Unit
void CPU() {
	int totalCycles = 0;
	unsigned insAddr = 0;

	ushort secs = 0;
	auto start = std::chrono::high_resolution_clock::now();

	printf(" --- CPU Running\n");
	RDY = true;
	while (PowerON) {
		while (RDY) {
			// Clock test -- should closely match one second. Set clk_test to true and verbose to false in order to use it.
			if (clkTest) {
				if (totalCycles >= cpu.CLOCK_SPEED) {
					secs++;
					totalCycles = 0;
					auto end = std::chrono::high_resolution_clock::now();
					std::cout << "CPU Speed: " << float(cpu.CLOCK_SPEED / 1000000.0) << "MHz" << " -- Executed in: " << "  " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
					start = std::chrono::high_resolution_clock::now();
				}
			}

			cpu.Cycles += cpu.CLOCK_SPEED / 20;
			totalCycles += cpu.CLOCK_SPEED / 20;

			while (cpu.Cycles >= 0) {
				if (!RES) { // Reset Sequence
					cpu.reset();
					RAMReset();
					if (verbose) {
						printf(" ---- RESET ----\n");
					}
				}
				insAddr = cpu.PC; // <-- Used for displaying the address of the current instruction's OPCODE
				cpu.FetchInstruction();
				cpu.Execute();
				if (verbose) {
					printf("PC: %04x    Ins: %02x    X: %02x    Y: %02x    AC: %02x    SR: %02x    SP: %02x    SP Val.: %02x    Ref. Addr.: %04x    Val. in Addr.: %02x\n", insAddr, cpu.IR, cpu.X, cpu.Y, cpu.AC, cpu.SR, cpu.SP, Memory[0x100 | cpu.SP], cpu.Address, Memory[cpu.Address]);
				}
				switch (cpu.CheckInterrupts()) {
					case 1:
						if (verbose) {
							printf("### IRQ Interrupt\n");
						}
						break;
					case 2:
						if (verbose) {
							printf("### NMI Interrupt\n");
						}
						break;
				}
				if (SDLStatus == false) {
					PowerON = false;
					RDY = false;
					break;
				}
			}
			std::this_thread::sleep_for(std::chrono::microseconds(50000));
		}
	}
}

int main(int argc, char* argv[]) {
	if (argc == 1) {
		std::cerr << "Error: Not enough arguments.\n";
		return 0;
	}
	else if (argc == 2) {
		if (strcmp(argv[1], "-help") == 0) {
			printf("Proper syntax: EVM -rom <path> -storage <path> <flag>\n\n");
			printf("Flags:          Description:\n");
			printf("  -rom          Path to ROM\n");
			printf("  -storage      Path to Virtual Storage Device (.img file)\n");
			printf("  -v            Enable Verbose\n");
			printf("  -clk          Enable Clock Test");
			printf("\n\nNotice: Verbose and Clock Test cannot be enabled at the same time.\n");
			return 0;
		}
		else if (strcmp(argv[1], "-about") == 0) {
			printf("EVM (Erick's Virtual Machine)\n * Version: %s\n", Version);
			return 0;
		}
		else if (strcmp(argv[1], "-rom") == 0) {
			printf("Error: Missing arguments! Use -help for more information.\n");
		}
		else {
			printf("Error: Invalid argument! Use -help for more information.\n");
		}
		return 1;
	}
	else if (argc == 3) {
		printf("Error: Missing arguments! Use -help for more information.\n");
		return 1;
	}
	else if (argc == 4) {
		if (strcmp(argv[3], "-storage") == 0) {
			printf("Error: Missing arguments! Use -help for more information.\n");
		}
		else {
			printf("Error: Invalid argument! Use -help for more information.\n");
		}
		return 1;
	}
	else if (argc == 5) {
		if ((strcmp(argv[3], "-storage") != 0) && (strcmp(argv[1], "-rom") != 0)) {
			printf("Error: Invalid arguments! Use -help for more information.\n");
			return 1;
		}
	}
	else if (argc == 6) {
		if (strcmp(argv[5], "-v") == 0) {
			verbose = true;
		}
		else if (strcmp(argv[5], "-clk") == 0) {
			clkTest = true;
		}
		else {
			std::cerr << "Error: Invalid flag!" << argv[5] << std::endl;
			return 1;
		}
	}
	else if (argc > 6) {
		printf("Error: Too many arguments!\n");
	}

	// Loading ROM from file
	std::vector<char> ROM(ROM_SIZE); // Contents of the ROM file
	std::ifstream rom(argv[2], std::ios::binary); // ROM file

	if (!rom.read(ROM.data(), ROM_SIZE)) {
		std::cerr << "Couldn't read the ROM file.\nPath is invalid, ROM file is smaller than " << ROM_SIZE / 1024 << "kb or permission to file is denied.\n";
		return 1;
	}
	rom.close();

	// Initializing Memory and Copying ROM
	ushort x = 0;
	for (unsigned int i = 0; i < 0x10000; i++) {
		if (i > (0xFFFF - ROM_SIZE)) {
			Memory[i] = ROM[x];
			x++;
		}
		else {
			Memory[i] = 0x00;
		}
	}

	ROM.clear();
	ROM.shrink_to_fit();

	viaOne.activationRange = 0x3FF0; // <- $3FF0-$3FFF
	viaTwo.activationRange = 0x3FE0; // <- $3FE0-$3FEF
	viaThree.activationRange = 0x3FD0; // <- $3FD0-$3FDF
	if (!ssd.initializeStorage(argv[4])) {
		return 1;
	}

	std::thread CPU_thread(CPU);
	std::thread VCU_thread(VCU);
	printf("Erick's Virtual Machine\n\n");
	printf("Version: %s\n", Version);

	VCU_thread.join();
	printf(" --- Stopping Emulation...\n");
	CPU_thread.join();
	return 0;
}
