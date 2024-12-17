// VIA (Versatile Interface Adapter) 6522
class VIA6522 {
private:
	unsigned char ret = 0; // General Return Value

public:
	unsigned char PA = 0; // Port A
	unsigned char PB = 0; // Port B
	unsigned char ORA = 0; // Output Register A
	unsigned char ORB = 0; // Outuput Register B
	unsigned char IRA = 0; // Input Register A
	unsigned char IRB = 0; // Input Register B
	unsigned char DDRA = 0; // Data Direction Register A
	unsigned char DDRB = 0; // Data Direction Register B
	unsigned char IFR = 0; // Interrupt Flags Register -- 7: IRQ, 6: Timer1, 5: Timer2, 4: CB1, 3: CB2, 2: Shift Register, 1: CA1, 0: CA2
	unsigned char IER = 0; // Interrupt Enable Register -- 7: Set/Clear, 6: Timer1, 5: Timer2, 4: CB1, 3: CB2, 2: Shift Register, 1: CA1, 0: CA2
	unsigned char PCR = 0; // Peripheral Control Register -- 0: CA1 Control, 1-3: CA2 Control, 4: CB1 Control, 5-7: CB2 Control
	bool CA1 = false, CA2 = false; // Port A Control Line
	bool CB1 = false, CB2 = false; // Port B Control Line
	unsigned short activationRange = 0; // Range of 15 addresses that activates the chip (Value is the first of these addresses)

	// Set IFR
	void setInterrupt() {
		if ((IER & 0b10000000) != 0) { // Set bits will enable interrupts for that line
			if ((IER & 0b00000010) != 0 && (PCR & 0b00000001) == 1 && (CA1)) { // Register Detected CA1 Interrupt
				IRA = PA & (~(DDRA));
				IFR = IFR | 0b00000010;
			}
			else if ((IER & 0b00010000) != 0 && (PCR & 0b00010000) == 1 && (CB1)) { // Register Detected CB1 Interrupt
				IRB = PB & (~(DDRB));
				IFR = IFR | 0b00010000;
			}
		}
		else if ((IER & 0b10000000) == 0) { // Set bits will disable interrupts for that line
			// CA1
			if ((IER & 0b00000010) == 0 && (PCR & 0b00000001) == 1 && (CA1)) { // Register Detected CA1 Interrupt
				IRA = PA & (~(DDRA));
				IFR = IFR | 0b00000010;
			}
			else if ((IER & 0b00010000) == 0 && (PCR & 0b00010000) == 1 && (CB1)) { // Register Detected CB1 Interrupt
				IRB = PB & (~(DDRB));
				IFR = IFR | 0b00010000;
			}
		}

		PA = PB = 0;
		if ((IFR & 0b01111111) != 0) { // There are Interrupt Flags Set
			IFR = IFR | 0b10000000;
		}
	}

	// Check if there are interrupts
	bool checkInterrupt() {
		if ((IFR & 0b10000000) == 0) {
			return true; // <- No interrupts
		}
		else {
			return false; // <- Trigger an interrupt (IRQ is active-low)
		}
	}

	// Send an instruction the 6522
	unsigned char sendInstruction(unsigned char r6522Ins, bool RW, unsigned char value) {
		switch (r6522Ins) {
			case 0x0: // Port B
				if (RW == 0) { // Write to ORB
					ORB = value;
					IFR = IFR & 0b11101111;
				}
				else { // Read from IRB
					IFR = IFR & 0b11101111;
					CB1 = false;
					ret = IRB;
					IRB = 0;
				}
				break;
			case 0x1: // Port A
				if (RW == 0) { // Write to ORA
					ORA = value;
					IFR = IFR & 0b11111101;
				}
				else { // Read from IRA
					IFR = IFR & 0b11111101;
					CA1 = false;
					ret = IRA;
					IRA = 0;
				}
				break;
			case 0x2: //DDRB (Data Direction Register B)
				if (RW == 0) { // Write to DDRB
					DDRB = value;
				}
				else { // Read DDRB
					ret = DDRB;
				}
				break;
			case 0x3: //DDRB (Data Direction Register B)
				if (RW == 0) { // Write to DDRA
					DDRA = value;
				}
				else { // Read DDRA
					ret = DDRA;
				}
				break;
			case 0xC: //PCR (Peripheral Control Register)
				if (RW == 0) { // Write to PCR
					PCR = value;
				}
				else { // Read PCR
					ret = PCR;
				}
				break;
			case 0xD:
				// ...
				break;
			case 0xE: //IER (Interrupt Enable Register) | (Enable/Disable Interrupts Lines)
				if (RW == 0) { // Set IER
					IER = value;
				}
				else { // Read IER
					ret = IER | 0b10000000;
				}
				break;
		}
		PA = ORA & DDRA;
		PB = ORB & DDRB;
		ORA = ORB = 0;
		if ((IFR & 0b01111111) == 0) {
			IFR = 0;
		}

		return ret;
	}
};
