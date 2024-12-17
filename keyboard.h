#define US 0
#define BR 1

// Convert ASCII values to USB Keyboard Values
unsigned char KeyTranslate(int ASCII, int LAYOUT_INDEX) {
	switch (LAYOUT_INDEX)
	{
		case US: // US Keyboard
			switch (ASCII) {
				case 0x08: return 0x2A; break; // BACKSPACE
				case 0x09: return 0x2B; break; // TAB
				case 0x0D: return 0x28; break; // ENTER
				case 0x1B: return 0x29; break; // ESC
				case 0x20: return 0x2C; break; // SPACEBAR
				case 0x27: return 0x34; break; // ' and "
				case 0x2C: return 0x36; break; // , and <
				case 0x2D: return 0x2D; break; // - and _
				case 0x2E: return 0x37; break; // . and >
				case 0x2F: return 0x38; break; // / and ?
				case 0x30: return 0x27; break; // 0 and )
				case 0x31: return 0x1E; break; // 1 and !
				case 0x32: return 0x1F; break; // 2 and @
				case 0x33: return 0x20; break; // 3 and #
				case 0x34: return 0x21; break; // 4 and $
				case 0x35: return 0x22; break; // 5 and %
				case 0x36: return 0x23; break; // 6 and ^
				case 0x37: return 0x24; break; // 7 and &
				case 0x38: return 0x25; break; // 8 and *
				case 0x39: return 0x26; break; // 9 and (
				case 0x3B: return 0x33; break; // ; and :
				case 0x3D: return 0x2E; break; // + and =
				case 0x5B: return 0x2F; break; // [ and {
				case 0x5C: return 0x31; break; // \ and |
				case 0x5D: return 0x30; break; // } and ]
				case 0x61: return 0x04; break; // A and a
				case 0x62: return 0x05; break; // B and b
				case 0x63: return 0x06; break; // C and c
				case 0x64: return 0x07; break; // D and d
				case 0x65: return 0x08; break; // E and e
				case 0x66: return 0x09; break; // F and f
				case 0x67: return 0x0A; break; // G and g
				case 0x68: return 0x0B; break; // H and h
				case 0x69: return 0x0C; break; // I and i
				case 0x6A: return 0x0D; break; // J and j
				case 0x6B: return 0x0E; break; // K and k
				case 0x6C: return 0x0F; break; // L and l
				case 0x6D: return 0x10; break; // M and m
				case 0x6E: return 0x11; break; // N and n
				case 0x6F: return 0x12; break; // O and o
				case 0x70: return 0x13; break; // P and p
				case 0x71: return 0x14; break; // Q and q
				case 0x72: return 0x15; break; // R and r
				case 0x73: return 0x16; break; // S and s
				case 0x74: return 0x17; break; // T and t
				case 0x75: return 0x18; break; // U and u
				case 0x76: return 0x19; break; // V and v
				case 0x77: return 0x1A; break; // W and w
				case 0x78: return 0x1B; break; // X and x
				case 0x79: return 0x1C; break; // Y and y
				case 0x7A: return 0x1D; break; // Z and z
				case 0x60: return 0x35; break; // ` and ~
				case 0x40000039: return 0x39; break; // Caps Lock
				case 0x4000003A: return 0x3A; break; // F1
				case 0x4000003B: return 0x3B; break; // F2
				case 0x4000003C: return 0x3C; break; // F3
				case 0x4000003D: return 0x3D; break; // F4
				case 0x4000003E: return 0x3E; break; // F5
				case 0x4000003F: return 0x3F; break; // F6
				case 0x40000040: return 0x40; break; // F7
				case 0x40000041: return 0x41; break; // F8
				case 0x40000042: return 0x42; break; // F9
				case 0x40000043: return 0x43; break; // F10
				case 0x40000044: return 0x44; break; // F11
				case 0x40000045: return 0x45; break; // F12
				case 0x4000004F: return 0x4F; break; // Arrow Right
				case 0x40000050: return 0x50; break; // Arrow Left
				case 0x40000051: return 0x51; break; // Arrow Down
				case 0x40000052: return 0x52; break; // Arrow Up
				default: return 0x00; break; // Unknown Key
			}
			break;
		case BR: // Brazilian Keyboard (ABNT2)
			switch (ASCII) {
				case 0x08: return 0x2A; break; // BACKSPACE
				case 0x09: return 0x2B; break; // TAB
				case 0x0D: return 0x28; break; // ENTER
				case 0x1B: return 0x29; break; // ESC
				case 0x20: return 0x2C; break; // SPACEBAR
				case 0x27: return 0x34; break; // ' and "
				case 0x2C: return 0x36; break; // , and <
				case 0x2D: return 0x2D; break; // - and _
				case 0x2E: return 0x37; break; // . and >
				case 0x2F: return 0x38; break; // / and ?
				case 0x30: return 0x27; break; // 0 and )
				case 0x31: return 0x1E; break; // 1 and !
				case 0x32: return 0x1F; break; // 2 and @
				case 0x33: return 0x20; break; // 3 and #
				case 0x34: return 0x21; break; // 4 and $
				case 0x35: return 0x22; break; // 5 and %
				case 0x36: return 0x23; break; // 6 and ¨
				case 0x37: return 0x24; break; // 7 and &
				case 0x38: return 0x25; break; // 8 and *
				case 0x39: return 0x26; break; // 9 and (
				case 0x3B: return 0x33; break; // ; and :
				case 0x3D: return 0x2E; break; // + and =
				case 0x5B: return 0x2F; break; // [ and {
				case 0x5C: return 0x31; break; // \ and |
				case 0x5D: return 0x30; break; // } and ]
				case 0x61: return 0x04; break; // A and a
				case 0x62: return 0x05; break; // B and b
				case 0x63: return 0x06; break; // C and c
				case 0x64: return 0x07; break; // D and d
				case 0x65: return 0x08; break; // E and e
				case 0x66: return 0x09; break; // F and f
				case 0x67: return 0x0A; break; // G and g
				case 0x68: return 0x0B; break; // H and h
				case 0x69: return 0x0C; break; // I and i
				case 0x6A: return 0x0D; break; // J and j
				case 0x6B: return 0x0E; break; // K and k
				case 0x6C: return 0x0F; break; // L and l
				case 0x6D: return 0x10; break; // M and m
				case 0x6E: return 0x11; break; // N and n
				case 0x6F: return 0x12; break; // O and o
				case 0x70: return 0x13; break; // P and p
				case 0x71: return 0x14; break; // Q and q
				case 0x72: return 0x15; break; // R and r
				case 0x73: return 0x16; break; // S and s
				case 0x74: return 0x17; break; // T and t
				case 0x75: return 0x18; break; // U and u
				case 0x76: return 0x19; break; // V and v
				case 0x77: return 0x1A; break; // W and w
				case 0x78: return 0x1B; break; // X and x
				case 0x79: return 0x1C; break; // Y and y
				case 0x7A: return 0x1D; break; // Z and z
				case 0x7E: return 0x35; break; // ~ and ^
				case 0x40000039: return 0x39; break; // Caps Lock
				case 0x4000003A: return 0x3A; break; // F1
				case 0x4000003B: return 0x3B; break; // F2
				case 0x4000003C: return 0x3C; break; // F3
				case 0x4000003D: return 0x3D; break; // F4
				case 0x4000003E: return 0x3E; break; // F5
				case 0x4000003F: return 0x3F; break; // F6
				case 0x40000040: return 0x40; break; // F7
				case 0x40000041: return 0x41; break; // F8
				case 0x40000042: return 0x42; break; // F9
				case 0x40000043: return 0x43; break; // F10
				case 0x40000044: return 0x44; break; // F11
				case 0x40000045: return 0x45; break; // F12
				case 0x4000004F: return 0x4F; break; // Arrow Right
				case 0x40000050: return 0x50; break; // Arrow Left
				case 0x40000051: return 0x51; break; // Arrow Down
				case 0x40000052: return 0x52; break; // Arrow Up
				default: return 0x00; break; // Unknown Key
			}
			break;
	}
}
