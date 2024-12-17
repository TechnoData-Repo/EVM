#include <iostream>
#include <vector>
#include <fstream>

class SSD {
private:
	unsigned int addressBus = 0; // Address Bus
	bool RW = 0; // Read/Write
	unsigned int AR = 0; // Address Register
	unsigned short OR = 0; // Offset Register
	unsigned short DSR = 0; // Destination/Source Register
	unsigned char storage[0x400000]; // Storage
	bool offsetSet = false; // OR is Set
	bool addressSet = false; // AR is Set
	bool dsrSet = false; // DSR is Set
	char SSDPath[100]; // Path to SSD Image File

	// Receive data from RAM and store it into SSD
	void receiveData() {
		extern VIA6522 viaTwo;
		extern unsigned char Memory[0x10000];
		extern bool IRQ;
		std::ofstream img;

		for (unsigned short i = 0; i < OR; i++) {
			storage[AR + i] = Memory[DSR + i];
		}

		img.open(SSDPath, std::ios::binary);
		if (img.is_open()) {
			for (unsigned int i = 0; i < 0x400000; i++) {
				img << storage[i];
			}
		}
		else {
			printf("Error: Couldn't write to SSD disk image.\n");
		}
		img.close();

		viaTwo.CA1 = true;
		viaTwo.setInterrupt();
		IRQ = viaTwo.checkInterrupt();
	}

	// Send data from SSD and store it into RAM
	void sendData() {
		extern VIA6522 viaTwo;
		extern unsigned char Memory[0x10000];
		extern bool IRQ;

		for (unsigned short i = 0; i < OR; i++) {
			Memory[DSR + i] = storage[AR + i];
		}
		viaTwo.CA1 = true;
		viaTwo.setInterrupt();
		IRQ = viaTwo.checkInterrupt();
	}

public:
	// Initialize Storage
	bool initializeStorage(char * path) {
		strcpy_s(SSDPath, _countof(SSDPath), path);

		std::vector<char> IMG(0x400000); // Contents of the Image file
		std::ifstream img(SSDPath, std::ios::binary); // Image file

		if (!img.read(IMG.data(), 0x400000)) {
			std::cerr << "Fatal Error: Couldn't load the SSD disk image!\n";
			return false;
		}
		img.close();

		for (unsigned int i = 0; i < 0x400000; i++) {
			storage[i] = IMG[i];
		}

		IMG.clear();
		IMG.shrink_to_fit();
		return true;
	}

	// Execute the Read or Write Instruction
	void executeInstruction() {
		if (dsrSet && addressSet && offsetSet) {
			if (RW == 1) {
				sendData();
			}
			else {
				receiveData();
			}
			RW = addressBus = 0;
			addressSet = offsetSet = false;
		}
	}

	// Latch Instructions and Set Up DMA
	bool latchAndSetUp(unsigned char value, unsigned char byte) {
		switch (byte) {
			case 0: // Byte 0
				addressBus = addressBus | value;
				break;
			case 1: // Byte 1
				addressBus = addressBus | value << 8;
				break;
			case 2: // Byte 2
				addressBus = addressBus | (value & 0b00111111) << 16;
				if ((value & 0b01000000) != 0) {
					RW = 1;
				}
				else {
					RW = 0;
				}
				if ((value & 0b10000000) != 0) {
					if (!dsrSet) {
						DSR = addressBus & 0xFFFF;
						addressBus = 0;
						dsrSet = true;
					}
					else if (!addressSet) {
						AR = addressBus & 0x3FFFFF;
						addressBus = 0;
						addressSet = true;
					}
					else if (!offsetSet) {
						OR = addressBus & 0x7FF;
						addressBus = 0;
						offsetSet = true;
						return true;
					}
				}
				break;
		}
		return false;
	}
};
