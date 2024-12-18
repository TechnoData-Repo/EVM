# You can use this script to generate an .img file of 4MB to use as a storage device with the emulator

img = bytearray([0x00] * 0x400000)

with open("ssd.img", "wb") as out_file:
	out_file.write(img)
