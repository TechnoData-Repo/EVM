class MOS65C02 {
public:
	// CPU Registers
	unsigned char AC = 0; // Accumulator Register
	unsigned char X = 0; // Register X
	unsigned char Y = 0; // Register Y
	unsigned char SR = 0; //Status Register
	unsigned short PC = 0; // Program Counter -- Address of the next instruction
	unsigned char SP = 0; // Stack Pointer -- Used as an offset from address 0x100 (Points from $100 to $1FF) -- ie Page 1
	
	int Cycles = 0; // CPU cycles availiable
	const int CLOCK_SPEED = 4000000; // CPU Clock Speed in Cycles
	unsigned short Address = 0; // Full memory address to be used by the instruction
	unsigned char Ins = 0; // Current Instruction

private:
	// Internal Variables
	unsigned char page = 0; // Page where last byte of the instruction occured -- used for reference in instructions that take an additional cycle when page boundary gets crossed
	unsigned char tmpVal = 0; // Temporary Value
	const int ROM_RANGE[2] = { 0xC000,0xFFFF }; // Read-Only Memory (ROM) Range
	
	// --- ADDRESSING MODES ---
	// Addressing Mode: (abs) "Indirect" -- $LLHH
	unsigned short addr_ind() {
		extern unsigned char Memory[0x10000];
		Address = (Memory[PC + 1] << 8) | Memory[PC];
		return (Memory[Address + 1] << 8) | Memory[Address];
	}
	// Addressing Mode: zpg "Zero Page" -- $LL
	unsigned char addr_zpg() {
		extern unsigned char Memory[0x10000];
		return Memory[PC];
	}
	// Addressing Mode: (zpg) "Zero Page, Indirect" -- ($LL)
	unsigned short addr_zpg_ind() {
		extern unsigned char Memory[0x10000];
		return (Memory[Memory[PC] + 1] << 8) | Memory[Memory[PC]];
	}
	// Addressing Mode: (X,ind) "X-indexed, Indirect" -- ($LL,X)
	unsigned short addr_x_ind() {
		extern unsigned char Memory[0x10000];
		return (Memory[(Memory[PC] + X + 1) & 0xFF] << 8) | Memory[(Memory[PC] + X) & 0xFF];
	}
	// Addressing Mode: abs "Absolute" -- $LLHH
	unsigned short addr_abs() {
		extern unsigned char Memory[0x10000];
		return (Memory[PC + 1] << 8) | Memory[PC];
	}
	// Addressing Mode: abs(ind,X) "Absolute, X-indexed, Indirect" -- ($LLHH,X)
	void addr_abs_ind() {
		extern unsigned char Memory[0x10000];
		page = (PC + 1) >> 8;
		Address = ((Memory[PC + 1] << 8) | Memory[PC]) + X;
		Address = (Memory[Address + 1] << 8) | Memory[Address];
	}
	// Addressing Mode: (ind),Y "Indirect, Y-indexed" -- ($LL),Y
	void addr_ind_y() {
		extern unsigned char Memory[0x10000];
		page = PC >> 8;
		Address = ((Memory[Memory[PC] + 1] << 8) | Memory[Memory[PC]]) + Y;
		if (page != (Address >> 8)) {
			Cycles--;
		}
	}
	// Addressing Mode: zpg,X "Zero Page, X-indexed" -- $LL,X
	unsigned short addr_zpg_x() {
		extern unsigned char Memory[0x10000];
		return (Memory[PC] + X) & 0xFF;
	}
	// Addressing Mode: zpg,Y "Zero Page, Y-indexed" -- $LL,Y
	unsigned short addr_zpg_y() {
		extern unsigned char Memory[0x10000];
		return (Memory[PC] + Y) & 0xFF;
	}
	// Addressing Mode: abs,Y "Absolute, Y-indexed" -- $LLHH,Y (Note: Page crossing affects cycle!)
	void addr_abs_y() {
		extern unsigned char Memory[0x10000];
		page = (PC + 1) >> 8;
		Address = ((Memory[PC + 1] << 8) | Memory[PC]) + Y;
		if (page != (Address >> 8)) {
			Cycles--;
		}
	}
	// Addressing Mode: abs,X "Absolute, X-indexed" -- $LLHH,X (Note: Page crossing affects cycle!)
	void addr_abs_x() {
		extern unsigned char Memory[0x10000];
		page = (PC + 1) >> 8;
		Address = ((Memory[PC + 1] << 8) | Memory[PC]) + X;
		if (page != (Address >> 8)) {
			Cycles--;
		}
	}

	// --- ALU STUFF ---
	// Read the value from a memory address
	unsigned char mem_read(unsigned short address) {
		extern unsigned char Memory[0x10000];
		extern VIA6522 viaOne, viaTwo, viaThree;
		unsigned char ret = 0;
		if (address >= viaOne.activationRange && address <= viaOne.activationRange + 0x0F) {
			ret = viaOne.sendInstruction((address & 0x0F), 1, NULL);
			return ret;
		}
		else if (address >= viaTwo.activationRange && address <= viaTwo.activationRange + 0x0F) {
			ret = viaTwo.sendInstruction((address & 0x0F), 1, NULL);
			return ret;
		}
		else if (address >= viaThree.activationRange && address <= viaThree.activationRange + 0x0F) {
			ret = viaThree.sendInstruction((address & 0x0F), 1, NULL);
			return ret;
		}
		else {
			return Memory[address];
		}
	}
	// Store the value of a register to Memory
	void mem_store(unsigned short address, unsigned char value) {
		extern unsigned char Memory[0x10000];
		extern bool verbose;
		extern VIA6522 viaOne, viaTwo, viaThree;
		extern SSD ssd;
		if (address >= viaOne.activationRange && address <= viaOne.activationRange + 0x0F) {
			viaOne.sendInstruction((address & 0x0F), 0, value);
		}
		else if (address >= viaTwo.activationRange && address <= viaTwo.activationRange + 0x0F) {
			viaTwo.sendInstruction((address & 0x0F), 0, value);
			if (viaTwo.PA != 0) {
				ssd.latchAndSetUp(viaTwo.PA, 0);
				viaTwo.PA = 0;
			}
			else if (viaTwo.PB != 0) {
				ssd.latchAndSetUp(viaTwo.PB, 1);
				viaTwo.PB = 0;
			}
		}
		else if (address >= viaThree.activationRange && address <= viaThree.activationRange + 0x0F) {
			viaThree.sendInstruction((address & 0x0F), 0, value);
			if (viaThree.PA != 0) {
				if (ssd.latchAndSetUp(viaThree.PA, 2)) {
					extern void SSD_RW();
					std::thread executeSSDInstruction(SSD_RW);
					executeSSDInstruction.detach();
				}
				viaThree.PA = 0;
			}
		}
		else {
			Memory[Address] = value;
		}
	}
	// Checks if last bit is set
	void check_negative(unsigned char value) {
		if ((SR & 0b00001000) != 0) {
			SR = SR & 0b01111111;
			Cycles--;
		}
		else {
			if ((value & 0b10000000) != 0) {
				SR = SR | 0b10000000;
			}
			else {
				SR = SR & 0b01111111;
			}
		}
	}
	// Checks if all bits are zeroes
	void check_zero(unsigned char value) {
		if (value == 0) {
			SR = SR | 0b00000010;
		}
		else {
			SR = SR & 0b11111101;
		}
	}
	// Addition with carry
	void addwcarry(unsigned char value) {
		if ((AC < 0x80 && ((AC + value + (SR & 0b00000001)) & 0xFF) < 0x80) || (AC >= 0x80 && ((AC + value + (SR & 0b00000001)) & 0xFF) >= 0x80)) {
			SR = SR & 0b10111111;
		}
		else {
			SR = SR | 0b01000000;
		}
		if ((AC + value + (SR & 0b00000001)) > 0xFF) {
			AC += (value + (SR & 0b00000001));
			SR = SR | 0b00000001;
		}
		else {
			AC += (value + (SR & 0b00000001));
			SR = SR & 0b11111110;
		}

		if ((SR & 0b00001000) != 0) {
			if ((AC & 0b00001111) > 9) {
				AC += 0x06;
			}
			if (((AC & 0b11110000) >> 4) > 9) {
				AC += 0x60;
				SR = SR | 0b00000001;
				Cycles--;
			}

			if ((SR & 0b01000000) != 0) {
				SR = SR & 0b10111111;
				Cycles--;
			}
		}
	}
	// Subtract with carry
	void subwcarry(unsigned char value) {
		if ((AC < 0x80 && ((AC - value - (SR & 0b00000001)) & 0xFF) < 0x80) || (AC >= 0x80 && ((AC - value - (SR & 0b00000001)) & 0xFF) >= 0x80)) {
			SR = SR & 0b10111111;
		}
		else {
			SR = SR | 0b01000000;
		}
		if (((value + (SR & 0b00000001)) & 0x1FF) > AC) {
			AC -= (value + (SR & 0b00000001));
			SR = SR | 0b00000001;
		}
		else {
			AC -= (value + (SR & 0b00000001));
			SR = SR & 0b11111110;
		}

		if ((SR & 0b00001000) != 0) {
			if ((AC & 0b00001111) > 9) {
				AC -= 0x06;
			}
			if (((AC & 0b11110000) >> 4) > 9) {
				AC -= 0x60;
				SR = SR | 0b00000001;
				Cycles--;
			}

			if ((SR & 0b01000000) != 0) {
				SR = SR & 0b10111111;
				Cycles--;
			}
		}
	}
	// Rotate Right
	void ror(unsigned short address) {
		if ((tmpVal & 0b00000001) == 0) {
			tmpVal = (tmpVal >> 1) | ((SR & 0b00000001) << 7);
			SR = SR & 0b11111110;
		}
		else {
			tmpVal = (tmpVal >> 1) | ((SR & 0b00000001) << 7);
			SR = SR | 0b00000001;
		}
		mem_store(address, tmpVal);
	}
	// Rotate Left
	void rol(unsigned short address) {
		if (((tmpVal << 1) & 0xFF) >= tmpVal) {
			tmpVal = (tmpVal << 1) | (SR & 0b00000001);
			SR = SR & 0b11111110;
		}
		else {
			tmpVal = (tmpVal << 1) | (SR & 0b00000001);
			SR = SR | 0b00000001;
		}
		mem_store(address, tmpVal);
	}
	// Arithmetic Shift Left
	void asl(unsigned short address) {
		if (((tmpVal << 1) & 0xFF) >= tmpVal) {
			SR = SR & 0b11111110;
		}
		else {
			SR = SR | 0b00000001;
		}
		tmpVal = tmpVal << 1;
		mem_store(address, tmpVal);
	}
	// Logical Shift Right
	void lsr(unsigned short address) {
		if ((tmpVal & 0b00000001) == 0) {
			SR = SR & 0b11111110;
		}
		else {
			SR = SR | 0b00000001;
		}
		tmpVal = tmpVal >> 1;
		mem_store(address, tmpVal);
	}
	// Compare Register with Memory
	void compare(unsigned char reg, unsigned short address) {
		unsigned char value = mem_read(address);
		if (reg >= value) {
			SR = SR | 0b00000001;
		}
		else {
			SR = SR & 0b11111110;
		}
		if (reg == value) {
			SR = SR | 0b00000010;
		}
		else {
			SR = SR & 0b11111101;
		}
		if (reg < value) {
			SR = SR | 0b10000000;
		}
		else {
			SR = SR & 0b01111111;
		}
	}
	// Push 8-bit value to the stack
	void push(unsigned char value) {
		extern unsigned char Memory[0x10000];
		SP--;
		Memory[0x100 | SP] = value;
	}

	// Pull 8-bit value from the stack
	unsigned char pull() {
		extern unsigned char Memory[0x10000];
		unsigned char value = Memory[0b100000000 | SP];
		Memory[0b100000000 | SP] = 0;
		SP++;
		return value;
	}
public:
	// Initiate Reset Sequence for MOS65C02
	void reset() {
		extern unsigned char Memory[0x10000];
		extern bool RES;
		
		SR = 0b00100100;
		AC = X = Y = 0;
		SP = 0xFF;
		PC = (Memory[0xFFFD] << 8) | Memory[0xFFFC]; // <- Setting PC to address of the Reset Vector (RES)
		RES = true;
		Cycles -= 7;
	}

	// Fetch instruction to be executed
	void FetchInstruction() {
		extern unsigned char Memory[0x10000];
		Ins = Memory[PC];
		PC++;
	}

	// Checks if there are hardware interrupts occurring
	unsigned char CheckInterrupts() {
		extern unsigned char Memory[0x10000];
		extern bool IRQ;
		extern bool NMI;
		// Interrupt Request from the IRQ pin
		if (((SR & 0b00000100) == 0) && (IRQ == false)) {
			push(PC >> 8);
			push(PC & 0xFF);
			push((SR | 0b00100000) & 0b11101111);
			SR = SR | 0b00000100;
			PC = (Memory[0xFFFF] << 8) | Memory[0xFFFE];
			Cycles -= 7;
			return 1;
		}
		// Interrupt Request from the NMI pin
		if (NMI == false) {
			push(PC >> 8);
			push(PC & 0xFF);
			push((SR | 0b00100000) & 0b11101111);
			SR = SR | 0b00000100;
			PC = (Memory[0xFFFB] << 8) | Memory[0xFFFA];
			Cycles -= 7;
			return 2;
		}
		return 0;
	}

	// Execute fetched instruction
	void Execute() {
		extern unsigned char Memory[0x10000];
		extern bool IRQ;
		extern VIA6522 viaOne;
		Address = page = 0;

		switch (Ins) {
			case 0x00: // BRK
				push((PC + 1) >> 8);
				push((PC + 1) & 0xFF);
				push(SR | 0b00110000);
				PC = (Memory[0xFFFF] << 8) | Memory[0xFFFE];
				Cycles -= 7;
				break;
			case 0x01: // ORA X, ind
				Address = addr_x_ind();
				AC = AC | mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 6;
				break;
			case 0x02: // NOP
				PC++;
				Cycles -= 2;
				break;
			case 0x03: // NOP
				Cycles -= 1;
				break;
			case 0x04: // TSB zpg
				Address = addr_abs();
				tmpVal = mem_read(Address);
				check_zero(AC & tmpVal);
				mem_store(Address, (tmpVal | AC));
				PC++;
				Cycles -= 5;
				break;
			case 0x05: // ORA zpg
				Address = addr_zpg();
				AC = AC | mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 3;
				break;
			case 0x06: // ASL zpg
				Address = addr_zpg();
				tmpVal = mem_read(Address);
				asl(Address);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC++;
				Cycles -= 5;
				break;
			case 0x07: // NOP
				Cycles -= 1;
				break;
			case 0x08: // PHP
				push(SR | 0b00110000);
				Cycles -= 3;
				break;
			case 0x09: // ORA immediate
				AC = AC | Memory[PC]; Address = PC;
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 2;
				break;
			case 0x0A: // ASL A
				if (((AC << 1) & 0xFF) >= AC) {
					SR = SR & 0b11111110;
				}
				else {
					SR = SR | 1;
				}
				AC = AC << 1;
				check_negative(AC);
				check_zero(AC);
				Cycles -= 2;
				break;
			case 0x0B: // NOP
				Cycles -= 1;
				break;
			case 0x0C: // TSB abs
				Address = addr_abs();
				tmpVal = mem_read(Address);
				check_zero(AC & tmpVal);
				mem_store(Address, (tmpVal | AC));
				PC += 2;
				Cycles -= 6;
				break;
			case 0x0D: // ORA abs
				Address = addr_abs();
				AC = AC | mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0x0E: // ASL abs
				Address = addr_abs();
				tmpVal = mem_read(Address);
				asl(Address);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC += 2;
				Cycles -= 6;
				break;
			case 0x0F: // NOP
				Cycles -= 1;
				break;
			case 0x10: // BPL
				if ((SR & 0b10000000) == 0) {
					page = PC >> 8;
					PC = PC + signed char(Memory[PC]);
				}
				PC++; Address = PC;
				if ((PC >> 8) != page) {
					Cycles -= 2;
				}
				else {
					Cycles--;
				}
				Cycles -= 2;
				break;
			case 0x11: // ORA ind, Y
				addr_ind_y();
				AC = AC | mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 5;
				break;
			case 0x12: // ORA (zpg)
				Address = addr_zpg_ind();
				AC = AC | mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 5;
				break;
			case 0x13: // NOP
				Cycles -= 1;
				break;
			case 0x14: // TRB zpg
				Address = addr_zpg();
				tmpVal = mem_read(Address);
				check_zero(AC & tmpVal);
				mem_store(Address, (tmpVal & (AC ^ 0xFF)));
				PC++;
				Cycles -= 5;
				break;
			case 0x15: // ORA zpg, X
				Address = addr_zpg_x();
				AC = AC | mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 4;
				break;
			case 0x16: // ASL zpg, X
				Address = addr_zpg_x();
				tmpVal = mem_read(Address);
				asl(Address);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC++;
				Cycles -= 6;
				break;
			case 0x17: // NOP
				Cycles -= 1;
				break;
			case 0x18: // CLC
				SR = SR & 0b11111110;
				Cycles -= 2;
				break;
			case 0x19: // ORA abs, Y
				addr_abs_y();
				AC = AC | mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0x1A: // INA
				AC++;
				check_negative(AC);
				check_zero(AC);
				Cycles -= 2;
				break;
			case 0x1B: // NOP
				Cycles -= 1;
				break;
			case 0x1C: // TRB abs
				Address = addr_abs();
				tmpVal = mem_read(Address);
				check_zero(AC & tmpVal);
				mem_store(Address, (tmpVal & (AC ^ 0xFF)));
				PC += 2;
				Cycles -= 6;
				break;
			case 0x1D: // ORA abs, X
				addr_abs_x();
				AC = AC | mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0x1E: // ASL abs, X
				Address = ((Memory[PC + 1] << 8) | Memory[PC]) + X; // <- Page boundary crossing does not affect this instruction
				tmpVal = mem_read(Address);
				asl(Address);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC += 2;
				Cycles -= 7;
				break;
			case 0x1F: // NOP
				Cycles -= 1;
				break;
			case 0x20: // JSR
				push((PC + 2) >> 8);
				push((PC + 2) & 0xFF);
				Address = PC = addr_abs();
				Cycles -= 6;
				break;
			case 0x21: // AND X, ind
				Address = addr_x_ind();
				AC = AC & mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 6;
				break;
			case 0x22: // NOP
				PC++;
				Cycles -= 2;
				break;
			case 0x23: // NOP
				Cycles -= 1;
				break;
			case 0x24: // BIT zpg
				Address = addr_zpg();
				tmpVal = mem_read(Address);
				SR = SR | (tmpVal & 0b10000000);
				SR = SR | (tmpVal & 0b01000000);
				check_zero(AC & tmpVal);
				PC++;
				Cycles -= 3;
				break;
			case 0x25: // AND zpg
				Address = addr_zpg();
				AC = AC & mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 3;
				break;
			case 0x26: // ROL zpg
				Address = addr_zpg();
				tmpVal = mem_read(Address);
				rol(Address);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC++;
				Cycles -= 5;
				break;
			case 0x27: // NOP
				Cycles -= 1;
				break;
			case 0x28: // PLP
				SR = pull() & 0b11001111;
				Cycles -= 4;
				break;
			case 0x29: // AND immediate
				AC = AC & Memory[PC]; Address = PC;
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 2;
				break;
			case 0x2A: // ROL A
				if (((AC << 1) & 0xFF) >= AC) {
					AC = (AC << 1) | (SR & 0b00000001);
					SR = SR & 0b11111110;
				}
				else {
					AC = (AC << 1) | (SR & 0b00000001);
					SR = SR | 0b00000001;
				}
				check_negative(AC);
				check_zero(AC);
				Cycles -= 2;
				break;
			case 0x2B: // NOP
				Cycles -= 1;
				break;
			case 0x2C: // BIT abs
				Address = addr_abs();
				tmpVal = mem_read(Address);
				SR = SR | (tmpVal & 0b10000000);
				SR = SR | (tmpVal & 0b01000000);
				check_zero(AC & tmpVal);
				PC += 2;
				Cycles -= 4;
				break;
			case 0x2D: // AND abs
				Address = addr_abs();
				AC = AC & mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0x2E: // ROL abs
				Address = addr_abs();
				tmpVal = mem_read(Address);
				rol(Address);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC += 2;
				Cycles -= 6;
				break;
			case 0x2F: // NOP
				Cycles -= 1;
				break;
			case 0x30: // BMI
				if ((SR & 0b10000000) != 0) {
					page = PC >> 8;
					PC = PC + signed char(Memory[PC]);
				}
				PC++; Address = PC;
				if ((PC >> 8) != page) {
					Cycles -= 2;
				}
				else {
					Cycles--;
				}
				Cycles -= 2;
				break;
			case 0x31: // AND ind, Y
				addr_ind_y();
				AC = AC & mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 5;
				break;
			case 0x32: // AND (zpg)
				Address = addr_zpg_ind();
				AC = AC & mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 5;
				break;
			case 0x33: // NOP
				Cycles -= 1;
				break;
			case 0x34: // BIT zpg,X
				Address = addr_zpg_x();
				tmpVal = mem_read(Address);
				SR = SR | (tmpVal & 0b10000000);
				SR = SR | (tmpVal & 0b01000000);
				check_zero(AC & tmpVal);
				PC++;
				Cycles -= 4;
				break;
			case 0x35: // AND zpg, X
				Address = addr_zpg_x();
				AC = AC & mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 4;
				break;
			case 0x36: // ROL zpg, X
				Address = addr_zpg_x();
				tmpVal = mem_read(Address);
				rol(Address);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC++;
				Cycles -= 6;
				break;
			case 0x37: // NOP
				Cycles -= 1;
				break;
			case 0x38: // SEC
				SR = SR | 0b00000001;
				Cycles -= 2;
				break;
			case 0x39: // AND abs, Y
				addr_abs_y();
				AC = AC & mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0x3A: // DEA
				AC--;
				check_negative(AC);
				check_zero(AC);
				Cycles -= 2;
				break;
			case 0x3B: // NOP
				Cycles -= 1;
				break;
			case 0x3C: // BIT abs,X
				addr_abs_x();
				tmpVal = mem_read(Address);
				SR = SR | (tmpVal & 0b10000000);
				SR = SR | (tmpVal & 0b01000000);
				check_zero(AC & tmpVal);
				PC += 2;
				Cycles -= 4;
				break;
			case 0x3D: // AND abs, X
				addr_abs_x();
				AC = AC & mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0x3E: // ROL abs, X
				Address = ((Memory[PC + 1] << 8) | Memory[PC]) + X; // <- Page boundary crossing does not affect this instruction
				tmpVal = mem_read(Address);
				rol(Address);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC += 2;
				Cycles -= 7;
				break;
			case 0x3F: // NOP
				Cycles -= 1;
				break;
			case 0x40: // RTI
				SR = pull() & 0b11001111;
				Address = PC = pull() | (pull() << 8);
				Cycles -= 6;
				IRQ = viaOne.checkInterrupt();
				break;
			case 0x41: // EOR X, ind
				Address = addr_x_ind();
				AC = AC ^ mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 6;
				break;
			case 0x42: // NOP
				PC++;
				Cycles -= 2;
				break;
			case 0x43: // NOP
				Cycles -= 1;
				break;
			case 0x44: // NOP
				PC++;
				Cycles -= 3;
				break;
			case 0x45: // EOR zpg
				Address = addr_zpg();
				AC = AC ^ mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 3;
				break;
			case 0x46: // LSR zpg
				Address = addr_zpg();
				tmpVal = mem_read(Address);
				lsr(Address);
				check_zero(tmpVal);
				SR = SR & 0b01111111;
				PC++;
				Cycles -= 5;
				break;
			case 0x47: // NOP
				Cycles -= 1;
				break;
			case 0x48: // PHA
				push(AC);
				Cycles -= 3;
				break;
			case 0x49: // EOR immediate
				AC = AC ^ Memory[PC]; Address = PC;
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 2;
				break;
			case 0x4A: // LSR A
				if ((AC & 0b00000001) == 0) {
					SR = SR & 0b11111110;
				}
				else {
					SR = SR | 1;
				}
				AC = AC >> 1;
				check_zero(AC);
				SR = SR & 0b01111111;
				Cycles -= 2;
				break;
			case 0x4B: // NOP
				Cycles -= 1;
				break;
			case 0x4C: // JMP abs
				Address = PC = addr_abs();
				Cycles -= 3;
				break;
			case 0x4D: // EOR abs
				Address = addr_abs();
				AC = AC ^ mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0x4E: // LSR abs
				Address = addr_abs();
				tmpVal = mem_read(Address);
				lsr(Address);
				check_zero(tmpVal);
				SR = SR & 0b01111111;
				PC += 2;
				Cycles -= 6;
				break;
			case 0x4F: // NOP
				Cycles -= 1;
				break;
			case 0x50: // BVC
				if ((SR & 0b01000000) == 0) {
					page = PC >> 8;
					PC = PC + signed char(Memory[PC]);
				}
				PC++; Address = PC;
				if ((PC >> 8) != page) {
					Cycles -= 2;
				}
				else {
					Cycles--;
				}
				Cycles -= 2;
				break;
			case 0x51: // EOR ind, Y
				addr_ind_y();
				AC = AC ^ mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 5;
				break;
			case 0x52: // EOR (zpg)
				Address = addr_zpg_ind();
				AC = AC ^ mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 5;
				break;
			case 0x53: // NOP
				Cycles -= 1;
				break;
			case 0x54: // NOP
				PC++;
				Cycles -= 4;
				break;
			case 0x55: // EOR zpg, X
				Address = addr_zpg_x();
				AC = AC ^ mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 4;
				break;
			case 0x56: // LSR zpg, X
				Address = addr_zpg_x();
				tmpVal = mem_read(Address);
				lsr(Address);
				check_zero(tmpVal);
				SR = SR & 0b01111111;
				PC++;
				Cycles -= 6;
				break;
			case 0x57: // NOP
				Cycles -= 1;
				break;
			case 0x58: // CLI
				SR = SR & 0b11111011;
				Cycles -= 2;
				break;
			case 0x59: // EOR abs, Y
				addr_abs_y();
				AC = AC ^ mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0x5A: // PHY
				push(Y);
				Cycles -= 3;
				break;
			case 0x5B: // NOP
				Cycles -= 1;
				break;
			case 0x5C: // NOP
				PC += 2;
				Cycles -= 8;
				break;
			case 0x5D: // EOR abs, X
				addr_abs_x();
				AC = AC ^ mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0x5E: // LSR abs, X
				Address = ((Memory[PC + 1] << 8) | Memory[PC]) + X; // <- Page boundary crossing does not affect this instruction
				tmpVal = mem_read(Address);
				lsr(Address);
				check_zero(tmpVal);
				SR = SR & 0b01111111;
				PC += 2;
				Cycles -= 7;
				break;
			case 0x5F: // NOP
				Cycles -= 1;
				break;
			case 0x60: // RTS
				Address = PC = pull() | (pull() << 8);
				Cycles -= 6;
				break;
			case 0x61: // ADC X, ind
				Address = addr_x_ind();
				addwcarry(mem_read(Address));
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 6;
				break;
			case 0x62: // NOP
				PC++;
				Cycles -= 2;
				break;
			case 0x63: // NOP
				Cycles -= 1;
				break;
			case 0x64: // STZ zpg
				Address = addr_zpg();
				mem_store(Address, 0x00);
				PC++;
				Cycles -= 3;
				break;
			case 0x65: // ADC zpg
				Address = addr_zpg();
				addwcarry(mem_read(Address));
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 3;
				break;
			case 0x66: // ROR zpg
				Address = addr_zpg();
				tmpVal = mem_read(Address);
				ror(Address);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC++;
				Cycles -= 5;
				break;
			case 0x67: // NOP
				Cycles -= 1;
				break;
			case 0x68: // PLA
				AC = pull();
				Cycles -= 4;
				break;
			case 0x69: // ADC immediate
				Address = PC;
				addwcarry(Memory[PC]);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 2;
				break;
			case 0x6A: // ROR A
				if ((AC & 0b00000001) == 0) {
					AC = (AC >> 1) | ((SR & 0b00000001) << 7);
					SR = SR & 0b11111110;
				}
				else {
					AC = (AC >> 1) | ((SR & 0b00000001) << 7);
					SR = SR | 0b00000001;
				}
				check_negative(AC);
				check_zero(AC);
				Cycles -= 2;
				break;
			case 0x6B: // NOP
				Cycles -= 1;
				break;
			case 0x6C: // JMP (abs)
				Address = PC = addr_ind();
				Cycles -= 6;
				break;
			case 0x6D: // ADC abs
				Address = addr_abs();
				addwcarry(mem_read(Address));
				check_negative(AC);
				check_zero(AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0x6E: // ROR abs
				Address = addr_abs();
				tmpVal = mem_read(Address);
				ror(Address);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC += 2;
				Cycles -= 6;
				break;
			case 0x6F: // NOP
				Cycles -= 1;
				break;
			case 0x70: // BVS
				if ((SR & 0b01000000) != 0) {
					page = PC >> 8;
					PC = PC + signed char(Memory[PC]);
				}
				PC++; Address = PC;
				if ((PC >> 8) != page) {
					Cycles -= 2;
				}
				else {
					Cycles--;
				}
				Cycles -= 2;
				break;
			case 0x71: // ADC ind, Y
				addr_ind_y();
				addwcarry(mem_read(Address));
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 5;
				break;
			case 0x72: // ADC (zpg)
				Address = addr_zpg_ind();
				addwcarry(mem_read(Address));
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 5;
				break;
			case 0x73: // NOP
				Cycles -= 1;
				break;
			case 0x74: // STZ zpg,X
				Address = addr_zpg_x();
				mem_store(Address, 0x00);
				PC++;
				Cycles -= 4;
				break;
			case 0x75: // ADC zpg, X
				Address = addr_zpg_x();
				addwcarry(mem_read(Address));
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 4;
				break;
			case 0x76: // ROR zpg, X
				Address = addr_zpg_x();
				tmpVal = mem_read(Address);
				ror(Address);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC++;
				Cycles -= 6;
				break;
			case 0x77: // NOP
				Cycles -= 1;
				break;
			case 0x78: // SEI
				SR = SR | 0b00000100;
				Cycles -= 2;
				break;
			case 0x79: // ADC abs, Y
				addr_abs_y();
				addwcarry(mem_read(Address));
				check_negative(AC);
				check_zero(AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0x7A: // PLY
				Y = pull();
				Cycles -= 4;
				break;
			case 0x7B: // NOP
				Cycles -= 1;
				break;
			case 0x7C: // JMP abs (ind,x)
				addr_abs_ind(); PC = Address;
				Cycles -= 3;
				break;
			case 0x7D: // ADC abs, X
				addr_abs_x();
				addwcarry(mem_read(Address));
				check_negative(AC);
				check_zero(AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0x7E: // ROR abs, X
				Address = ((Memory[PC + 1] << 8) | Memory[PC]) + X; // <- Page boundary crossing does not affect this instruction
				tmpVal = mem_read(Address);
				ror(Address);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC += 2;
				Cycles -= 7;
				break;
			case 0x7F: // NOP
				Cycles -= 1;
				break;
			case 0x80: // BRA
				page = PC >> 8;
				PC = PC + signed char(Memory[PC]) + 1; Address = PC;
				if ((PC >> 8) != page) {
					Cycles -= 2;
				}
				else {
					Cycles--;
				}
				Cycles -= 2;
				break;
			case 0x81: // STA X, ind
				Address = addr_x_ind();
				mem_store(Address, AC);
				PC++;
				Cycles -= 6;
				break;
			case 0x82: // NOP
				PC++;
				Cycles -= 2;
				break;
			case 0x83: // NOP
				Cycles -= 1;
				break;
			case 0x84: // STY zpg
				Address = addr_zpg();
				mem_store(Address, Y);
				PC++;
				Cycles -= 3;
				break;
			case 0x85: // STA zpg
				Address = addr_zpg();
				mem_store(Address, AC);
				PC++;
				Cycles -= 3;
				break;
			case 0x86: // STX zpg
				Address = addr_zpg();
				mem_store(Address, X);
				PC++;
				Cycles -= 3;
				break;
			case 0x87: // NOP
				Cycles -= 1;
				break;
			case 0x88: // DEY
				Y--;
				check_negative(Y);
				check_zero(Y);
				Cycles -= 2;
				break;
			case 0x89: // BIT immediate
				tmpVal = Memory[PC]; Address = PC;
				check_zero(AC & tmpVal);
				PC++;
				Cycles -= 2;
				break;
			case 0x8A: // TXA
				AC = X;
				check_negative(AC);
				check_zero(AC);
				Cycles -= 2;
				break;
			case 0x8B: // NOP
				Cycles -= 1;
				break;
			case 0x8C: // STY abs
				Address = addr_abs();
				mem_store(Address, Y);
				PC += 2;
				Cycles -= 4;
				break;
			case 0x8D: // STA abs
				Address = addr_abs();
				mem_store(Address, AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0x8E: // STX abs
				Address = addr_abs();
				mem_store(Address, X);
				PC += 2;
				Cycles -= 4;
				break;
			case 0x8F: // NOP
				Cycles -= 1;
				break;
			case 0x90: // BCC
				if ((SR & 0b00000001) == 0) {
					page = PC >> 8;
					PC = PC + signed char(Memory[PC]);
				}
				PC++; Address = PC;
				if ((PC >> 8) != page) {
					Cycles -= 2;
				}
				else {
					Cycles--;
				}
				Cycles -= 2;
				break;
			case 0x91: // STA ind, Y
				Address = ((Memory[Memory[PC] + 1] << 8) | Memory[Memory[PC]]) + Y; // <- Page boundary crossing does not affect this instruction
				mem_store(Address, AC);
				PC++;
				Cycles -= 6;
				break;
			case 0x92: // STA (zpg)
				Address = addr_zpg_ind();
				mem_store(Address, AC);
				PC++;
				Cycles -= 5;
				break;
			case 0x93: // NOP
				Cycles -= 1;
				break;
			case 0x94: // STY zpg, X
				Address = addr_zpg_x();
				mem_store(Address, Y);
				PC++;
				Cycles -= 4;
				break;
			case 0x95: // STA zpg, X
				Address = addr_zpg_x();
				mem_store(Address, AC);
				PC++;
				Cycles -= 4;
				break;
			case 0x96: // STX zpg, Y
				Address = addr_zpg_y();
				mem_store(Address, X);
				PC++;
				Cycles -= 4;
				break;
			case 0x97: // NOP
				Cycles -= 1;
				break;
			case 0x98: // TYA
				AC = Y;
				check_negative(AC);
				check_zero(AC);
				Cycles -= 2;
				break;
			case 0x99: // STA abs, Y
				Address = ((Memory[PC + 1] << 8) | Memory[PC]) + Y;
				mem_store(Address, AC);
				PC += 2;
				Cycles -= 5;
				break;
			case 0x9A: // TXS
				SP = X;
				Cycles -= 2;
				break;
			case 0x9B: // NOP
				Cycles -= 1;
				break;
			case 0x9C: // STZ abs
				Address = addr_abs();
				mem_store(Address, 0x00);
				PC += 2;
				Cycles -= 4;
				break;
			case 0x9D: // STA abs, X
				Address = ((Memory[PC + 1] << 8) | Memory[PC]) + X;
				mem_store(Address, AC);
				PC += 2;
				Cycles -= 5;
				break;
			case 0x9E: // STZ abs, X
				Address = ((Memory[PC + 1] << 8) | Memory[PC]) + X;
				mem_store(Address, 0x00);
				PC += 2;
				Cycles -= 5;
				break;
			case 0x9F: // NOP
				Cycles -= 1;
				break;
			case 0xA0: // LDY immediate
				Y = Memory[PC]; Address = PC;
				check_negative(Y);
				check_zero(Y);
				PC++;
				Cycles -= 2;
				break;
			case 0xA1: // LDA X, ind
				Address = addr_x_ind();
				AC = mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 6;
				break;
			case 0xA2: // LDX immediate
				X = Memory[PC]; Address = PC;
				check_negative(X);
				check_zero(X);
				PC++;
				Cycles -= 2;
				break;
			case 0xA3: // NOP
				Cycles -= 1;
				break;
			case 0xA4: // LDY zpg
				Address = addr_zpg();
				Y = mem_read(Address);
				check_negative(Y);
				check_zero(Y);
				PC++;
				Cycles -= 3;
				break;
			case 0xA5: // LDA zpg
				Address = addr_zpg();
				AC = mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 3;
				break;
			case 0xA6: // LDX zpg
				Address = addr_zpg();
				X = mem_read(Address);
				check_negative(X);
				check_zero(X);
				PC++;
				Cycles -= 3;
				break;
			case 0xA7: // NOP
				Cycles -= 1;
				break;
			case 0xA8: // TAY
				Y = AC;
				check_negative(Y);
				check_zero(Y);
				Cycles -= 2;
				break;
			case 0xA9: // LDA immediate
				AC = Memory[PC]; Address = PC;
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 2;
				break;
			case 0xAA: // TAX
				X = AC;
				check_negative(X);
				check_zero(X);
				Cycles -= 2;
				break;
			case 0xAB: // NOP
				Cycles -= 1;
				break;
			case 0xAC: // LDY abs
				Address = addr_abs();
				Y = mem_read(Address);
				check_negative(Y);
				check_zero(Y);
				PC += 2;
				Cycles -= 4;
				break;
			case 0xAD: // LDA abs
				Address = addr_abs();
				AC = mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0xAE: // LDX abs
				Address = addr_abs();
				X = mem_read(Address);
				check_negative(X);
				check_zero(X);
				PC += 2;
				Cycles -= 4;
				break;
			case 0xAF: // NOP
				Cycles -= 1;
				break;
			case 0xB0: // BCS
				if ((SR & 0b00000001) == 1) {
					page = PC >> 8;
					PC = PC + signed char(Memory[PC]);
				}
				PC++; Address = PC;
				if ((PC >> 8) != page) {
					Cycles -= 2;
				}
				else {
					Cycles--;
				}
				Cycles -= 2;
				break;
			case 0xB1: // LDA ind, Y
				addr_ind_y();
				AC = mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 5;
				break;
			case 0xB2: // LDA (zpg)
				Address = addr_zpg_ind();
				AC = mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 5;
				break;
			case 0xB3: // NOP
				Cycles -= 1;
				break;
			case 0xB4: // LDY zpg, X
				Address = addr_zpg_x();
				Y = mem_read(Address);
				check_negative(Y);
				check_zero(Y);
				PC++;
				Cycles -= 4;
				break;
			case 0xB5: // LDA zpg, X
				Address = addr_zpg_x();
				AC = mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 4;
				break;
			case 0xB6: // LDX zpg, Y
				Address = addr_zpg_y();
				X = mem_read(Address);
				check_negative(X);
				check_zero(X);
				PC++;
				Cycles -= 4;
				break;
			case 0xB7: // NOP
				Cycles -= 1;
				break;
			case 0xB8: // CLV
				SR = SR & 0b10111111;
				Cycles -= 2;
				break;
			case 0xB9: // LDA abs, Y
				addr_abs_y();
				AC = mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0xBA: // TSX
				X = SP;
				check_negative(X);
				check_zero(X);
				Cycles -= 2;
				break;
			case 0xBB: // NOP
				Cycles -= 1;
				break;
			case 0xBC: // LDY abs, X
				addr_abs_x();
				Y = mem_read(Address);
				check_negative(Y);
				check_zero(Y);
				PC += 2;
				Cycles -= 4;
				break;
			case 0xBD: // LDA abs, X
				addr_abs_x();
				AC = mem_read(Address);
				check_negative(AC);
				check_zero(AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0xBE: // LDX abs, Y
				addr_abs_y();
				X = mem_read(Address);
				check_negative(X);
				check_zero(X);
				PC += 2;
				Cycles -= 4;
				break;
			case 0xBF: // NOP
				Cycles -= 1;
				break;
			case 0xC0: // CPY immediate
				compare(Y, PC); Address = PC;
				PC++;
				Cycles -= 2;
				break;
			case 0xC1: // CMP X, ind
				Address = addr_x_ind();
				compare(AC, Address);
				PC++;
				Cycles -= 6;
				break;
			case 0xC2: // NOP
				PC++;
				Cycles -= 2;
				break;
			case 0xC3: // NOP
				Cycles -= 1;
				break;
			case 0xC4: // CPY zpg
				Address = addr_zpg();
				compare(Y, Address);
				PC++;
				Cycles -= 3;
				break;
			case 0xC5: // CMP zpg
				Address = addr_zpg();
				compare(AC, Address);
				PC++;
				Cycles -= 3;
				break;
			case 0xC6: // DEC zpg
				Address = addr_zpg();
				tmpVal = mem_read(Address) - 1;
				mem_store(Address, tmpVal);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC++;
				Cycles -= 5;
				break;
			case 0xC7: // NOP
				Cycles -= 1;
				break;
			case 0xC8: // INY
				Y++;
				check_negative(Y);
				check_zero(Y);
				Cycles -= 2;
				break;
			case 0xC9: // CMP immediate
				compare(AC, PC); Address = PC;
				PC++;
				Cycles -= 2;
				break;
			case 0xCA: // DEX
				X--;
				check_negative(X);
				check_zero(X);
				Cycles -= 2;
				break;
			case 0xCB: // NOP
				Cycles -= 1;
				break;
			case 0xCC: // CPY abs
				Address = addr_abs();
				compare(Y, Address);
				PC += 2;
				Cycles -= 4;
				break;
			case 0xCD: // CMP abs
				Address = addr_abs();
				compare(AC, Address);
				PC += 2;
				Cycles -= 4;
				break;
			case 0xCE: // DEC abs
				Address = addr_abs();
				tmpVal = mem_read(Address) - 1;
				mem_store(Address, tmpVal);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC += 2;
				Cycles -= 6;
				break;
			case 0xCF: // NOP
				Cycles -= 1;
				break;
			case 0xD0: // BNE
				if ((SR & 0b00000010) == 0) {
					page = PC >> 8;
					PC = PC + signed char(Memory[PC]);
				}
				PC++; Address = PC;
				if ((PC >> 8) != page) {
					Cycles -= 2;
				}
				else {
					Cycles--;
				}
				Cycles -= 2;
				break;
			case 0xD1: // CMP ind, Y
				addr_ind_y();
				compare(AC, Address);
				PC++;
				Cycles -= 5;
				break;
			case 0xD2: // CMP (zpg)
				Address = addr_zpg_ind();
				compare(AC, Address);
				PC++;
				Cycles -= 5;
				break;
			case 0xD3: // NOP
				Cycles -= 1;
				break;
			case 0xD4: // NOP
				PC++;
				Cycles -= 4;
				break;
			case 0xD5: // CMP zpg, X
				Address = addr_zpg_x();
				compare(AC, Address);
				PC++;
				Cycles -= 4;
				break;
			case 0xD6: // DEC zpg, X
				Address = addr_zpg_x();
				tmpVal = mem_read(Address) - 1;
				mem_store(Address, tmpVal);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC++;
				Cycles -= 6;
				break;
			case 0xD7: // NOP
				Cycles -= 1;
				break;
			case 0xD8: // CLD
				SR = SR & 0b11110111;
				Cycles -= 2;
				break;
			case 0xD9: // CMP abs, Y
				addr_abs_y();
				compare(AC, Address);
				PC += 2;
				Cycles -= 4;
				break;
			case 0xDA: // PHX
				push(X);
				Cycles -= 3;
				break;
			case 0xDB: // NOP
				Cycles -= 1;
				break;
			case 0xDC: // NOP
				PC += 2;
				Cycles -= 4;
				break;
			case 0xDD: // CMP abs, X
				addr_abs_x();
				compare(AC, Address);
				PC += 2;
				Cycles -= 4;
				break;
			case 0xDE: // DEC abs, X
				Address = ((Memory[PC + 1] << 8) | Memory[PC]) + X; // <- Page boundary crossing does not affect this instruction
				tmpVal = mem_read(Address) - 1;
				mem_store(Address, tmpVal);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC += 2;
				Cycles -= 7;
				break;
			case 0xDF: // NOP
				Cycles -= 1;
				break;
			case 0xE0: // CPX immediate
				compare(X, PC); Address = PC;
				PC++;
				Cycles -= 2;
				break;
			case 0xE1: // SBC X, ind
				Address = addr_x_ind();
				subwcarry(mem_read(Address));
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 6;
				break;
			case 0xE2: // NOP
				PC++;
				Cycles -= 2;
				break;
			case 0xE3: // NOP
				Cycles -= 1;
				break;
			case 0xE4: // CPX zpg
				Address = addr_zpg();
				compare(X, Address);
				PC++;
				Cycles -= 3;
				break;
			case 0xE5: // SBC zpg
				Address = addr_zpg();
				subwcarry(mem_read(Address));
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 3;
				break;
			case 0xE6: // INC zpg
				Address = addr_zpg();
				tmpVal = mem_read(Address) + 1;
				mem_store(Address, tmpVal);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC++;
				Cycles -= 5;
				break;
			case 0xE7: // NOP
				Cycles -= 1;
				break;
			case 0xE8: // INX
				X++;
				check_negative(X);
				check_zero(X);
				Cycles -= 2;
				break;
			case 0xE9: // SBC immediate
				subwcarry(Memory[PC]); Address = PC;
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 2;
				break;
			case 0xEA: // NOP
				Cycles -= 2;
				break;
			case 0xEB: // NOP
				Cycles -= 1;
				break;
			case 0xEC: // CPX abs
				Address = addr_abs();
				compare(X, Address);
				PC += 2;
				Cycles -= 4;
				break;
			case 0xED: // SBC abs
				Address = addr_abs();
				subwcarry(mem_read(Address));
				check_negative(AC);
				check_zero(AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0xEE: // INC abs
				Address = addr_abs();
				tmpVal = mem_read(Address) + 1;
				mem_store(Address, tmpVal);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC += 2;
				Cycles -= 6;
				break;
			case 0xEF: // NOP
				Cycles -= 1;
				break;
			case 0xF0: // BEQ
				if ((SR & 0b00000010) != 0) {
					page = PC >> 8;
					PC = PC + signed char(Memory[PC]);
				}
				PC++; Address = PC;
				if ((PC >> 8) != page) {
					Cycles -= 2;
				}
				else {
					Cycles--;
				}
				Cycles -= 2;
				break;
			case 0xF1: // SBC ind, Y
				addr_ind_y();
				subwcarry(mem_read(Address));
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 5;
				break;
			case 0xF2: // SBC (zpg)
				Address = addr_zpg_ind();
				subwcarry(mem_read(Address));
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 5;
				break;
			case 0xF3: // NOP
				Cycles -= 1;
				break;
			case 0xF4: // NOP
				PC++;
				Cycles -= 4;
				break;
			case 0xF5: // SBC zpg, X
				Address = addr_zpg_x();
				subwcarry(mem_read(Address));
				check_negative(AC);
				check_zero(AC);
				PC++;
				Cycles -= 4;
				break;
			case 0xF6: // INC zpg, X
				Address = addr_zpg_x();
				tmpVal = mem_read(Address) + 1;
				mem_store(Address, tmpVal);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC++;
				Cycles -= 6;
				break;
			case 0xF7: // NOP
				Cycles -= 1;
				break;
			case 0xF8: // SED
				SR = SR | 0b00001000;
				Cycles -= 2;
				break;
			case 0xF9: // SBC abs, Y
				addr_abs_y();
				subwcarry(mem_read(Address));
				check_negative(AC);
				check_zero(AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0xFA: // PLX
				X = pull();
				Cycles -= 4;
				break;
			case 0xFB: // NOP
				Cycles -= 1;
				break;
			case 0xFC: // NOP
				PC += 2;
				Cycles -= 4;
				break;
			case 0xFD: // SBC abs, X
				addr_abs_x();
				subwcarry(mem_read(Address));
				check_negative(AC);
				check_zero(AC);
				PC += 2;
				Cycles -= 4;
				break;
			case 0xFE: // INC abs, X
				addr_abs_x();
				tmpVal = mem_read(Address) + 1;
				mem_store(Address, tmpVal);
				check_negative(tmpVal);
				check_zero(tmpVal);
				PC += 2;
				Cycles -= 7;
				break;
			case 0xFF: // NOP
				Cycles -= 1;
				break;
		}
	}
};
