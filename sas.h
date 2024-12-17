class SASVCU {
private:
	unsigned char pixelsRemaining = 0; // Pixels remaining to be drawn on the current byte (4 colors mode)

public:
	// Video Control Unit (VCU) Registers 
	unsigned char HR = 0; // Horizontal Axis Register -- Controls the horizontal position of the next pixel to be decoded
	unsigned char VR = 0; // Vertical Axis Register -- Controls the vertical position of the pixel to be decoded
	unsigned short PXP = 0; // Pixel Pointer -- Points to the next pixel to be decoded
	unsigned char PXD = 0; // Pixel Data -- Pixel data being decoded	
	bool CMR = 1; // Color Mode Register -- Current Color Mode
	unsigned char RGB[3] = { 0,0,0 }; // RGB Color Code

	// Fetch Pixel data from Video Reserved Space in Memory
	void FetchPixelData() {
		extern unsigned char Memory[0x10000];
		switch (CMR) {
			case 0:
				PXD = Memory[PXP];
				break;
			case 1:
				switch (pixelsRemaining) {
					case 4:
						PXD = Memory[PXP] & 0b00000011;
						pixelsRemaining--;
						break;
					case 3:
						PXD = (Memory[PXP] & 0b00001100) >> 2;
						pixelsRemaining--;
						break;
					case 2:
						PXD = (Memory[PXP] & 0b00110000) >> 4;
						pixelsRemaining--;
						break;
					case 1:
						PXD = (Memory[PXP] & 0b11000000) >> 6;
						pixelsRemaining = 4;
						break;
				}
				break;
		}
	}

	// Increment Video Registers
	void IncVideoRegs() {
		switch (CMR) {
			case 0: // 256-Colors
				PXP = 0x4000 | ((PXP + 1) & 0x3FFF);
				HR++;

				switch (HR) {
					case 0x80:
						VR++;
						VR = VR & 0x7F;
						break;
				}
				HR = HR & 0x7F;
				break;
			case 1: // 4-Colors
				switch (HR + 1) {
					case 0x100:
						HR = 0;
						VR++;
						break;
					default:
						HR++;
						break;
				}

				switch (HR % 4) {
					case 0:
						PXP = 0x4000 | ((PXP + 1) & 0x3FFF);
						break;
				}
				break;
		}
	}

	// Translate the Pixel Data into RGB values -- Indexed Colors
	void TranslatePixel() {
		switch (CMR) {
			case 0: // 256-Colors
				switch (PXD) {
					case 0x00:
						RGB[0] = RGB[1] = RGB[2] = 0;
						break;
					case 0x01:
						RGB[0] = RGB[1] = RGB[2] = 37;
						break;
					case 0x02:
						RGB[0] = RGB[1] = RGB[2] = 52;
						break;
					case 0x03:
						RGB[0] = RGB[1] = RGB[2] = 78;
						break;
					case 0x04:
						RGB[0] = RGB[1] = RGB[2] = 104;
						break;
					case 0x05:
						RGB[0] = RGB[1] = RGB[2] = 117;
						break;
					case 0x06:
						RGB[0] = RGB[1] = RGB[2] = 142;
						break;
					case 0x07:
						RGB[0] = RGB[1] = RGB[2] = 164;
						break;
					case 0x08:
						RGB[0] = RGB[1] = RGB[2] = 184;
						break;
					case 0x09:
						RGB[0] = RGB[1] = RGB[2] = 197;
						break;
					case 0x0A:
						RGB[0] = RGB[1] = RGB[2] = 208;
						break;
					case 0x0B:
						RGB[0] = RGB[1] = RGB[2] = 215;
						break;
					case 0x0C:
						RGB[0] = RGB[1] = RGB[2] = 225;
						break;
					case 0x0D:
						RGB[0] = RGB[1] = RGB[2] = 234;
						break;
					case 0x0E:
						RGB[0] = RGB[1] = RGB[2] = 244;
						break;
					case 0x0F:
						RGB[0] = RGB[1] = RGB[2] = 255;
						break;
					case 0x10:
						RGB[0] = RGB[1] = 225; RGB[2] = 170;
						break;
					case 0x11:
						RGB[0] = RGB[1] = 225; RGB[2] = 144;
						break;
					case 0x12:
						RGB[0] = 255; RGB[1] = 249; RGB[2] = 112;
						break;
					case 0x13:
						RGB[0] = 255; RGB[1] = 244; RGB[2] = 86;
						break;
					case 0x14:
						RGB[0] = 255; RGB[1] = 230; RGB[2] = 81;
						break;
					case 0x15:
						RGB[0] = 255; RGB[1] = 216; RGB[2] = 76;
						break;
					case 0x16:
						RGB[0] = 255; RGB[1] = 208; RGB[2] = 59;
						break;
					case 0x17:
						RGB[0] = 255; RGB[1] = 197; RGB[2] = 31;
						break;
					case 0x18:
						RGB[0] = 255; RGB[1] = 171; RGB[2] = 29;
						break;
					case 0x19:
						RGB[0] = 255; RGB[1] = 145; RGB[2] = 26;
						break;
					case 0x1A:
						RGB[0] = 228; RGB[1] = 123; RGB[2] = 7;
						break;
					case 0x1B:
						RGB[0] = 195; RGB[1] = 104; RGB[2] = 6;
						break;
					case 0x1C:
						RGB[0] = 154; RGB[1] = 80; RGB[2] = 0;
						break;
					case 0x1D:
						RGB[0] = 118; RGB[1] = 55; RGB[2] = 0;
						break;
					case 0x1E:
						RGB[0] = 84; RGB[1] = 40; RGB[2] = 0;
						break;
					case 0x1F:
						RGB[0] = 65; RGB[1] = 32; RGB[2] = 0;
						break;
					case 0x20:
						RGB[0] = 69; RGB[1] = 25; RGB[2] = 4;
						break;
					case 0x21:
						RGB[0] = 114; RGB[1] = 30; RGB[2] = 17;
						break;
					case 0x22:
						RGB[0] = 159; RGB[1] = 36; RGB[2] = 30;
						break;
					case 0x23:
						RGB[0] = 179; RGB[1] = 58; RGB[2] = 32;
						break;
					case 0x24:
						RGB[0] = 200; RGB[1] = 81; RGB[2] = 32;
						break;
					case 0x25:
						RGB[0] = 227; RGB[1] = 105; RGB[2] = 32;
						break;
					case 0x26:
						RGB[0] = 252; RGB[1] = 129; RGB[2] = 32;
						break;
					case 0x27:
						RGB[0] = 253; RGB[1] = 140; RGB[2] = 37;
						break;
					case 0x28:
						RGB[0] = 254; RGB[1] = 152; RGB[2] = 44;
						break;
					case 0x29:
						RGB[0] = 255; RGB[1] = 174; RGB[2] = 56;
						break;
					case 0x2A:
						RGB[0] = 255; RGB[1] = 185; RGB[2] = 70;
						break;
					case 0x2B:
						RGB[0] = 255; RGB[1] = 191; RGB[2] = 81;
						break;
					case 0x2C:
						RGB[0] = 255; RGB[1] = 198; RGB[2] = 109;
						break;
					case 0x2D:
						RGB[0] = 255; RGB[1] = 213; RGB[2] = 135;
						break;
					case 0x2E:
						RGB[0] = 255; RGB[1] = 228; RGB[2] = 152;
						break;
					case 0x2F:
						RGB[0] = 255; RGB[1] = 230; RGB[2] = 171;
						break;
					case 0x30:
						RGB[0] = 255; RGB[1] = 218; RGB[2] = 208;
						break;
					case 0x31:
						RGB[0] = 255; RGB[1] = 208; RGB[2] = 195;
						break;
					case 0x32:
						RGB[0] = 255; RGB[1] = 194; RGB[2] = 178;
						break;
					case 0x33:
						RGB[0] = 255; RGB[1] = 179; RGB[2] = 158;
						break;
					case 0x34:
						RGB[0] = 255; RGB[1] = 164; RGB[2] = 139;
						break;
					case 0x35:
						RGB[0] = 255; RGB[1] = 152; RGB[2] = 124;
						break;
					case 0x36:
						RGB[0] = 255; RGB[1] = 138; RGB[2] = 106;
						break;
					case 0x37:
						RGB[0] = 253; RGB[1] = 120; RGB[2] = 84;
						break;
					case 0x38:
						RGB[0] = 243; RGB[1] = 110; RGB[2] = 74;
						break;
					case 0x39:
						RGB[0] = 231; RGB[1] = 98; RGB[2] = 62;
						break;
					case 0x3A:
						RGB[0] = 211; RGB[1] = 78; RGB[2] = 42;
						break;
					case 0x3B:
						RGB[0] = 191; RGB[1] = 54; RGB[2] = 36;
						break;
					case 0x3C:
						RGB[0] = 176; RGB[1] = 47; RGB[2] = 15;
						break;
					case 0x3D:
						RGB[0] = 152; RGB[1] = 44; RGB[2] = 14;
						break;
					case 0x3E:
						RGB[0] = 122; RGB[1] = 36; RGB[2] = 13;
						break;
					case 0x3F:
						RGB[0] = 93; RGB[1] = 31; RGB[2] = 12;
						break;
					case 0x40:
						RGB[0] = 74; RGB[1] = 23; RGB[2] = 0;
						break;
					case 0x41:
						RGB[0] = 114; RGB[1] = 31; RGB[2] = 0;
						break;
					case 0x42:
						RGB[0] = 168; RGB[1] = 19; RGB[2] = 0;
						break;
					case 0x43:
						RGB[0] = 200; RGB[1] = 33; RGB[2] = 10;
						break;
					case 0x44:
						RGB[0] = 223; RGB[1] = 37; RGB[2] = 18;
						break;
					case 0x45:
						RGB[0] = 236; RGB[1] = 59; RGB[2] = 36;
						break;
					case 0x46:
						RGB[0] = 250; RGB[1] = 82; RGB[2] = 54;
						break;
					case 0x47:
						RGB[0] = 252; RGB[1] = 97; RGB[2] = 72;
						break;
					case 0x48:
						RGB[0] = 255; RGB[1] = 112; RGB[2] = 95;
						break;
					case 0x49:
						RGB[0] = 255; RGB[1] = 126; RGB[2] = 126;
						break;
					case 0x4A:
						RGB[0] = 255; RGB[1] = 143; RGB[2] = 143;
						break;
					case 0x4B:
						RGB[0] = 255; RGB[1] = 157; RGB[2] = 158;
						break;
					case 0x4C:
						RGB[0] = 255; RGB[1] = 171; RGB[2] = 173;
						break;
					case 0x4D:
						RGB[0] = 255; RGB[1] = 185; RGB[2] = 189;
						break;
					case 0x4E:
						RGB[0] = 255; RGB[1] = 199; RGB[2] = 206;
						break;
					case 0x4F:
						RGB[0] = 255; RGB[1] = 202; RGB[2] = 222;
						break;
					case 0x50:
						RGB[0] = 255; RGB[1] = 184; RGB[2] = 236;
						break;
					case 0x51:
						RGB[0] = 255; RGB[1] = 175; RGB[2] = 234;
						break;
					case 0x52:
						RGB[0] = 255; RGB[1] = 165; RGB[2] = 231;
						break;
					case 0x53:
						RGB[0] = 255; RGB[1] = 157; RGB[2] = 229;
						break;
					case 0x54:
						RGB[0] = 255; RGB[1] = 141; RGB[2] = 225;
						break;
					case 0x55:
						RGB[0] = 251; RGB[1] = 126; RGB[2] = 218;
						break;
					case 0x56:
						RGB[0] = 239; RGB[1] = 114; RGB[2] = 206;
						break;
					case 0x57:
						RGB[0] = 228; RGB[1] = 103; RGB[2] = 195;
						break;
					case 0x58:
						RGB[0] = 215; RGB[1] = 90; RGB[2] = 182;
						break;
					case 0x59:
						RGB[0] = 202; RGB[1] = 77; RGB[2] = 169;
						break;
					case 0x5A:
						RGB[0] = 186; RGB[1] = 61; RGB[2] = 153;
						break;
					case 0x5B:
						RGB[0] = 170; RGB[1] = 34; RGB[2] = 136;
						break;
					case 0x5C:
						RGB[0] = 149; RGB[1] = 15; RGB[2] = 116;
						break;
					case 0x5D:
						RGB[0] = 128; RGB[1] = 3; RGB[2] = 95;
						break;
					case 0x5E:
						RGB[0] = 102; RGB[1] = 0; RGB[2] = 75;
						break;
					case 0x5F:
						RGB[0] = 73; RGB[1] = 0; RGB[2] = 54;
						break;
					case 0x60:
						RGB[0] = 72; RGB[1] = 3; RGB[2] = 108;
						break;
					case 0x61:
						RGB[0] = 92; RGB[1] = 4; RGB[2] = 136;
						break;
					case 0x62:
						RGB[0] = 101; RGB[1] = 13; RGB[2] = 144;
						break;
					case 0x63:
						RGB[0] = 123; RGB[1] = 35; RGB[2] = 167;
						break;
					case 0x64:
						RGB[0] = 147; RGB[1] = 59; RGB[2] = 191;
						break;
					case 0x65:
						RGB[0] = 157; RGB[1] = 69; RGB[2] = 201;
						break;
					case 0x66:
						RGB[0] = 167; RGB[1] = 79; RGB[2] = 211;
						break;
					case 0x67:
						RGB[0] = 178; RGB[1] = 90; RGB[2] = 222;
						break;
					case 0x68:
						RGB[0] = 189; RGB[1] = 101; RGB[2] = 233;
						break;
					case 0x69:
						RGB[0] = 197; RGB[1] = 109; RGB[2] = 241;
						break;
					case 0x6A:
						RGB[0] = 206; RGB[1] = 118; RGB[2] = 250;
						break;
					case 0x6B:
						RGB[0] = 213; RGB[1] = 131; RGB[2] = 255;
						break;
					case 0x6C:
						RGB[0] = 218; RGB[1] = 144; RGB[2] = 255;
						break;
					case 0x6D:
						RGB[0] = 222; RGB[1] = 156; RGB[2] = 255;
						break;
					case 0x6E:
						RGB[0] = 226; RGB[1] = 169; RGB[2] = 255;
						break;
					case 0x6F:
						RGB[0] = 230; RGB[1] = 182; RGB[2] = 255;
						break;
					case 0x70:
						RGB[0] = 205; RGB[1] = 211; RGB[2] = 255;
						break;
					case 0x71:
						RGB[0] = 192; RGB[1] = 203; RGB[2] = 255;
						break;
					case 0x72:
						RGB[0] = 175; RGB[1] = 190; RGB[2] = 255;
						break;
					case 0x73:
						RGB[0] = 159; RGB[1] = 178; RGB[2] = 255;
						break;
					case 0x74:
						RGB[0] = 151; RGB[1] = 169; RGB[2] = 255;
						break;
					case 0x75:
						RGB[0] = 144; RGB[1] = 160; RGB[2] = 255;
						break;
					case 0x76:
						RGB[0] = 128; RGB[1] = 145; RGB[2] = 255;
						break;
					case 0x77:
						RGB[0] = 113; RGB[1] = 131; RGB[2] = 255;
						break;
					case 0x78:
						RGB[0] = 101; RGB[1] = 117; RGB[2] = 255;
						break;
					case 0x79:
						RGB[0] = 90; RGB[1] = 104; RGB[2] = 255;
						break;
					case 0x7A:
						RGB[0] = 79; RGB[1] = 90; RGB[2] = 236;
						break;
					case 0x7B:
						RGB[0] = 68; RGB[1] = 76; RGB[2] = 222;
						break;
					case 0x7C:
						RGB[0] = 38; RGB[1] = 61; RGB[2] = 212;
						break;
					case 0x7D:
						RGB[0] = 8; RGB[1] = 47; RGB[2] = 202;
						break;
					case 0x7E:
						RGB[0] = 6; RGB[1] = 38; RGB[2] = 165;
						break;
					case 0x7F:
						RGB[0] = 5; RGB[1] = 30; RGB[2] = 129;
						break;
					case 0x80:
						RGB[0] = 11; RGB[1] = 7; RGB[2] = 121;
						break;
					case 0x81:
						RGB[0] = 32; RGB[1] = 28; RGB[2] = 142;
						break;
					case 0x82:
						RGB[0] = 53; RGB[1] = 49; RGB[2] = 163;
						break;
					case 0x83:
						RGB[0] = 70; RGB[1] = 66; RGB[2] = 180;
						break;
					case 0x84:
						RGB[0] = 87; RGB[1] = 83; RGB[2] = 197;
						break;
					case 0x85:
						RGB[0] = 97; RGB[1] = 93; RGB[2] = 207;
						break;
					case 0x86:
						RGB[0] = 109; RGB[1] = 105; RGB[2] = 219;
						break;
					case 0x87:
						RGB[0] = 123; RGB[1] = 119; RGB[2] = 233;
						break;
					case 0x88:
						RGB[0] = 137; RGB[1] = 133; RGB[2] = 247;
						break;
					case 0x89:
						RGB[0] = 145; RGB[1] = 141; RGB[2] = 255;
						break;
					case 0x8A:
						RGB[0] = 156; RGB[1] = 152; RGB[2] = 255;
						break;
					case 0x8B:
						RGB[0] = 167; RGB[1] = 164; RGB[2] = 255;
						break;
					case 0x8C:
						RGB[0] = 178; RGB[1] = 175; RGB[2] = 255;
						break;
					case 0x8D:
						RGB[0] = 187; RGB[1] = 184; RGB[2] = 255;
						break;
					case 0x8E:
						RGB[0] = 195; RGB[1] = 193; RGB[2] = 255;
						break;
					case 0x8F:
						RGB[0] = 211; RGB[1] = 209; RGB[2] = 255;
						break;
					case 0x90:
						RGB[0] = 192; RGB[1] = 235; RGB[2] = 255;
						break;
					case 0x91:
						RGB[0] = 180; RGB[1] = 226; RGB[2] = 255;
						break;
					case 0x92:
						RGB[0] = 159; RGB[1] = 212; RGB[2] = 255;
						break;
					case 0x93:
						RGB[0] = 141; RGB[1] = 218; RGB[2] = 255;
						break;
					case 0x94:
						RGB[0] = 130; RGB[1] = 211; RGB[2] = 255;
						break;
					case 0x95:
						RGB[0] = 116; RGB[1] = 203; RGB[2] = 255;
						break;
					case 0x96:
						RGB[0] = 105; RGB[1] = 202; RGB[2] = 255;
						break;
					case 0x97:
						RGB[0] = 85; RGB[1] = 182; RGB[2] = 255;
						break;
					case 0x98:
						RGB[0] = 78; RGB[1] = 168; RGB[2] = 236;
						break;
					case 0x99:
						RGB[0] = 72; RGB[1] = 155; RGB[2] = 217;
						break;
					case 0x9A:
						RGB[0] = 50; RGB[1] = 134; RGB[2] = 207;
						break;
					case 0x9B:
						RGB[0] = 29; RGB[1] = 113; RGB[2] = 198;
						break;
					case 0x9C:
						RGB[0] = 29; RGB[1] = 92; RGB[2] = 172;
						break;
					case 0x9D:
						RGB[0] = 29; RGB[1] = 72; RGB[2] = 146;
						break;
					case 0x9E:
						RGB[0] = 29; RGB[1] = 56; RGB[2] = 118;
						break;
					case 0x9F:
						RGB[0] = 29; RGB[1] = 41; RGB[2] = 90;
						break;
					case 0xA0:
						RGB[0] = 0; RGB[1] = 75; RGB[2] = 89;
						break;
					case 0xA1:
						RGB[0] = 0; RGB[1] = 93; RGB[2] = 110;
						break;
					case 0xA2:
						RGB[0] = 0; RGB[1] = 111; RGB[2] = 132;
						break;
					case 0xA3:
						RGB[0] = 0; RGB[1] = 132; RGB[2] = 156;
						break;
					case 0xA4:
						RGB[0] = 0; RGB[1] = 153; RGB[2] = 191;
						break;
					case 0xA5:
						RGB[0] = 0; RGB[1] = 171; RGB[2] = 202;
						break;
					case 0xA6:
						RGB[0] = 0; RGB[1] = 188; RGB[2] = 222;
						break;
					case 0xA7:
						RGB[0] = 0; RGB[1] = 208; RGB[2] = 245;
						break;
					case 0xA8:
						RGB[0] = 16; RGB[1] = 220; RGB[2] = 255;
						break;
					case 0xA9:
						RGB[0] = 62; RGB[1] = 225; RGB[2] = 255;
						break;
					case 0xAA:
						RGB[0] = 100; RGB[1] = 231; RGB[2] = 255;
						break;
					case 0xAB:
						RGB[0] = 118; RGB[1] = 234; RGB[2] = 255;
						break;
					case 0xAC:
						RGB[0] = 139; RGB[1] = 237; RGB[2] = 255;
						break;
					case 0xAD:
						RGB[0] = 154; RGB[1] = 239; RGB[2] = 255;
						break;
					case 0xAE:
						RGB[0] = 177; RGB[1] = 243; RGB[2] = 255;
						break;
					case 0xAF:
						RGB[0] = 199; RGB[1] = 246; RGB[2] = 255;
						break;
					case 0xB0:
						RGB[0] = 205; RGB[1] = 252; RGB[2] = 205;
						break;
					case 0xB1:
						RGB[0] = 195; RGB[1] = 249; RGB[2] = 195;
						break;
					case 0xB2:
						RGB[0] = 179; RGB[1] = 247; RGB[2] = 179;
						break;
					case 0xB3:
						RGB[0] = 153; RGB[1] = 242; RGB[2] = 153;
						break;
					case 0xB4:
						RGB[0] = 133; RGB[1] = 237; RGB[2] = 133;
						break;
					case 0xB5:
						RGB[0] = 124; RGB[1] = 228; RGB[2] = 124;
						break;
					case 0xB6:
						RGB[0] = 114; RGB[1] = 218; RGB[2] = 114;
						break;
					case 0xB7:
						RGB[0] = 81; RGB[1] = 205; RGB[2] = 81;
						break;
					case 0xB8:
						RGB[0] = 78; RGB[1] = 185; RGB[2] = 78;
						break;
					case 0xB9:
						RGB[0] = 54; RGB[1] = 164; RGB[2] = 54;
						break;
					case 0xBA:
						RGB[0] = 39; RGB[1] = 146; RGB[2] = 39;
						break;
					case 0xBB:
						RGB[0] = 24; RGB[1] = 128; RGB[2] = 24;
						break;
					case 0xBC:
						RGB[0] = 14; RGB[1] = 118; RGB[2] = 14;
						break;
					case 0xBD:
						RGB[0] = 3; RGB[1] = 107; RGB[2] = 3;
						break;
					case 0xBE:
						RGB[0] = 0; RGB[1] = 84; RGB[2] = 0;
						break;
					case 0xBF:
						RGB[0] = 0; RGB[1] = 72; RGB[2] = 0;
						break;
					case 0xC0:
						RGB[0] = 22; RGB[1] = 64; RGB[2] = 0;
						break;
					case 0xC1:
						RGB[0] = 28; RGB[1] = 83; RGB[2] = 0;
						break;
					case 0xC2:
						RGB[0] = 35; RGB[1] = 102; RGB[2] = 0;
						break;
					case 0xC3:
						RGB[0] = 40; RGB[1] = 120; RGB[2] = 0;
						break;
					case 0xC4:
						RGB[0] = 46; RGB[1] = 140; RGB[2] = 0;
						break;
					case 0xC5:
						RGB[0] = 58; RGB[1] = 152; RGB[2] = 12;
						break;
					case 0xC6:
						RGB[0] = 71; RGB[1] = 165; RGB[2] = 25;
						break;
					case 0xC7:
						RGB[0] = 81; RGB[1] = 175; RGB[2] = 35;
						break;
					case 0xC8:
						RGB[0] = 92; RGB[1] = 186; RGB[2] = 46;
						break;
					case 0xC9:
						RGB[0] = 113; RGB[1] = 207; RGB[2] = 67;
						break;
					case 0xCA:
						RGB[0] = 133; RGB[1] = 227; RGB[2] = 87;
						break;
					case 0xCB:
						RGB[0] = 141; RGB[1] = 235; RGB[2] = 95;
						break;
					case 0xCC:
						RGB[0] = 151; RGB[1] = 245; RGB[2] = 105;
						break;
					case 0xCD:
						RGB[0] = 160; RGB[1] = 254; RGB[2] = 114;
						break;
					case 0xCE:
						RGB[0] = 177; RGB[1] = 255; RGB[2] = 138;
						break;
					case 0xCF:
						RGB[0] = 188; RGB[1] = 255; RGB[2] = 154;
						break;
					case 0xD0:
						RGB[0] = 242; RGB[1] = 255; RGB[2] = 171;
						break;
					case 0xD1:
						RGB[0] = 232; RGB[1] = 252; RGB[2] = 121;
						break;
					case 0xD2:
						RGB[0] = 219; RGB[1] = 239; RGB[2] = 108;
						break;
					case 0xD3:
						RGB[0] = 205; RGB[1] = 225; RGB[2] = 83;
						break;
					case 0xD4:
						RGB[0] = 194; RGB[1] = 214; RGB[2] = 83;
						break;
					case 0xD5:
						RGB[0] = 184; RGB[1] = 204; RGB[2] = 73;
						break;
					case 0xD6:
						RGB[0] = 171; RGB[1] = 191; RGB[2] = 60;
						break;
					case 0xD7:
						RGB[0] = 158; RGB[1] = 178; RGB[2] = 47;
						break;
					case 0xD8:
						RGB[0] = 139; RGB[1] = 159; RGB[2] = 28;
						break;
					case 0xD9:
						RGB[0] = 121; RGB[1] = 141; RGB[2] = 10;
						break;
					case 0xDA:
						RGB[0] = 108; RGB[1] = 127; RGB[2] = 0;
						break;
					case 0xDB:
						RGB[0] = 96; RGB[1] = 113; RGB[2] = 0;
						break;
					case 0xDC:
						RGB[0] = 73; RGB[1] = 86; RGB[2] = 0;
						break;
					case 0xDD:
						RGB[0] = 68; RGB[1] = 82; RGB[2] = 0;
						break;
					case 0xDE:
						RGB[0] = 56; RGB[1] = 68; RGB[2] = 0;
						break;
					case 0xDF:
						RGB[0] = 44; RGB[1] = 53; RGB[2] = 0;
						break;
					case 0xE0:
						RGB[0] = 70; RGB[1] = 58; RGB[2] = 9;
						break;
					case 0xE1:
						RGB[0] = 77; RGB[1] = 63; RGB[2] = 9;
						break;
					case 0xE2:
						RGB[0] = 84; RGB[1] = 69; RGB[2] = 9;
						break;
					case 0xE3:
						RGB[0] = 108; RGB[1] = 88; RGB[2] = 9;
						break;
					case 0xE4:
						RGB[0] = 144; RGB[1] = 118; RGB[2] = 9;
						break;
					case 0xE5:
						RGB[0] = 171; RGB[1] = 139; RGB[2] = 10;
						break;
					case 0xE6:
						RGB[0] = 193; RGB[1] = 161; RGB[2] = 32;
						break;
					case 0xE7:
						RGB[0] = 208; RGB[1] = 176; RGB[2] = 47;
						break;
					case 0xE8:
						RGB[0] = 222; RGB[1] = 190; RGB[2] = 61;
						break;
					case 0xE9:
						RGB[0] = 230; RGB[1] = 198; RGB[2] = 69;
						break;
					case 0xEA:
						RGB[0] = 237; RGB[1] = 205; RGB[2] = 76;
						break;
					case 0xEB:
						RGB[0] = 245; RGB[1] = 216; RGB[2] = 98;
						break;
					case 0xEC:
						RGB[0] = 251; RGB[1] = 226; RGB[2] = 118;
						break;
					case 0xED:
						RGB[0] = 252; RGB[1] = 238; RGB[2] = 152;
						break;
					case 0xEE:
						RGB[0] = 253; RGB[1] = 243; RGB[2] = 169;
						break;
					case 0xEF:
						RGB[0] = 253; RGB[1] = 243; RGB[2] = 190;
						break;
					case 0xF0:
						RGB[0] = 255; RGB[1] = 218; RGB[2] = 150;
						break;
					case 0xF1:
						RGB[0] = 255; RGB[1] = 207; RGB[2] = 126;
						break;
					case 0xF2:
						RGB[0] = 255; RGB[1] = 202; RGB[2] = 105;
						break;
					case 0xF3:
						RGB[0] = 255; RGB[1] = 193; RGB[2] = 96;
						break;
					case 0xF4:
						RGB[0] = 252; RGB[1] = 183; RGB[2] = 92;
						break;
					case 0xF5:
						RGB[0] = 249; RGB[1] = 173; RGB[2] = 88;
						break;
					case 0xF6:
						RGB[0] = 237; RGB[1] = 160; RGB[2] = 78;
						break;
					case 0xF7:
						RGB[0] = 225; RGB[1] = 147; RGB[2] = 68;
						break;
					case 0xF8:
						RGB[0] = 208; RGB[1] = 133; RGB[2] = 58;
						break;
					case 0xF9:
						RGB[0] = 191; RGB[1] = 119; RGB[2] = 48;
						break;
					case 0xFA:
						RGB[0] = 181; RGB[1] = 100; RGB[2] = 39;
						break;
					case 0xFB:
						RGB[0] = 171; RGB[1] = 81; RGB[2] = 31;
						break;
					case 0xFC:
						RGB[0] = 141; RGB[1] = 58; RGB[2] = 19;
						break;
					case 0xFD:
						RGB[0] = 112; RGB[1] = 36; RGB[2] = 8;
						break;
					case 0xFE:
						RGB[0] = 88; RGB[1] = 31; RGB[2] = 5;
						break;
					case 0xFF:
						RGB[0] = 64; RGB[1] = 26; RGB[2] = 2;
						break;
				}
				break;
			case 1: // 4-Colors
				switch (PXD) {
					case 0x00:
						RGB[0] = RGB[1] = RGB[2] = 0;
						break;
					case 0x01:
						RGB[0] = RGB[1] = RGB[2] = 255;
						break;
					case 0x02:
						RGB[0] = RGB[1] = 255; RGB[2] = 0;
						break;
					case 0x03:
						RGB[0] = 255; RGB[1] = RGB[2] = 0;
						break;
				}
				break;
		}
	}

	// Reset Video Control Unit
	void reset() {
		extern VIA6522 viaTwo;
		PXP = 0x4000; // Setting PX to first address of System's Video Reserved Space
		CMR = !CMR;
		HR = VR = 0;
		if (CMR == 1) {
			pixelsRemaining = 4;
		}
	}
};
