#include "BMP.h"
#include <iostream>
#include <fstream>


BMP::BMP() {

}

BMP::~BMP() {
	if (lines){
		delete[] lines;
	}
}

bool BMP::read(const char* bmpFileName) {
	filePath = bmpFileName;

	std::ifstream file;
	file.open(bmpFileName, std::ios::binary);


	// Check if file is BMP
	char BM[2];
	file.read((char*)BM, 2);
	if (!BM[0] == 'B' && BM[1] == 'M') {
		return 0;
	}

	// Read file size
	file.read((char*)&fileSize, 4);

	// Reserved 
	file.seekg(4, std::ios::cur);

	// Pixel start position
	int pixelStartOffset = 0;
	file.read((char*)&pixelStartOffset, 4);

	// DIB header size
	int DIBHeaderSize = 0;
	file.read((char*)&DIBHeaderSize, 4);
	if (DIBHeaderSize != 40) {
		file.close();
		return 0;
	}

	// Picture width
	file.read((char*)&width, 4);

	// Picture height
	file.read((char*)&height, 4);

	lines = new Line[height];
	for (int y = 0; y < height; y++) {
		lines[y].createLine(width);
	}

	// Reserved 
	file.seekg(2, std::ios::cur);

	// Bits per pixelArray
	int bytesPerPixel = 0;
	file.read((char*)&bytesPerPixel, 2);
	bytesPerPixel /= 8;
	if (bytesPerPixel == 0) return 0;

	// Pixel save method
	int compressionType = 0;
	file.read((char*)&compressionType, 4);
	if (compressionType != 0) {
		file.close();
		return 0;
	}

	// Not used
	file.seekg(20, std::ios::cur);


	// Reading pixels
	int padding = width % 4;

	// reading
	char* line = new char[width * bytesPerPixel];

	for (ui32 y = 0; y < abs(height); y++) {
		file.read(line, width * bytesPerPixel);

		for (ui32 x = 0; x < width; x++) {
			ui8v3 p(0);

			if (bytesPerPixel == 3 || bytesPerPixel == 4) {
				p = { line[x * bytesPerPixel + 0], line[x * bytesPerPixel + 1], line[x * bytesPerPixel + 2] };
			}
			if (bytesPerPixel == 1) {
				p = { line[x], line[x], line[x] };
			}
			lines[height - y - 1][x] = p;
		}
		if (padding) {
			file.seekg(padding, std::ios::cur);
		}
	}
	delete[] line;

	if (height < 0) {
		height = -height;
	}
	file.close(); 

	return 1;
}

//
//bool BMP::create(const char* bmpFileName, long long bmpWidth, long long bmpHeight, unsigned char* source) {
//	const int fileHeaderSize = 14;
//	const int bmpHeaderV3Size = 40;
//	bitsPerPixel = 24;
//	width = bmpWidth;
//	height = bmpHeight;
//
//
//
//	std::ofstream bmp;
//	bmp.open(bmpFileName, std::ios::binary);
//
//
//
//	// Copying into local memory
//	unsigned char* arr = new unsigned char[width * height * 4];;
//	memcpy(arr, source, width * height * 4);
//	// Definig a bitmap file
//	bmp.write("BM", 2);
//
//
//
//	// Definig a file size
//	int padding = width % 4;
//	fileSize = fileHeaderSize + bmpHeaderV3Size + width * height * (bitsPerPixel / 8) + height * padding;
//	bmp.write((const char*)buff, 4);
//
//
//
//	// Reserved 
//	bmp.write("\0\0\0\0", 4);
//
//
//
//	// Pixel start position
//	bmp.write("6\0\0\0", 4); // 6 means 36 in hex
//
//
//
//	// DIB header size
//	bmp.write((char*)buff, 4);
//
//
//
//	// Picture width
//	bmp.write((char*)buff, 4);
//
//
//
//	// Picture height
//	bmp.write((char*)buff, 4);
//
//
//
//	// Reserved 
//	bmp.write("\01\0", 2);
//
//
//
//	// Bits per pixelArray
//	bmp.write((char*)buff, 2);
//
//
//
//	// Pixel save method
//	bmp.write("\0\0\0\0", 4);
//
//
//
//	// Pixel data size
//	bmp.write((char*)buff, 4);
//
//
//
//	// Not used
//	bmp.write("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 16);
//
//
//
//
//	//inverting current array
//	
//	if (needToInvertByHorizont) {
//		int pos = 0;
//		unsigned char tmp[3];
//		for (long long i = 0; i < height / 2; i++) {
//			for (long long j = 0; j < width; j++) {
//				pos = (i * width + j) * 4;
//
//				tmp[0] = arr[pos + 0];
//				tmp[1] = arr[pos + 1];
//				tmp[2] = arr[pos + 2];
//				arr[pos + 0] = arr[width * height * 4 - width * 4 * (i + 1) + j * 4 + 0];
//				arr[pos + 1] = arr[width * height * 4 - width * 4 * (i + 1) + j * 4 + 1];
//				arr[pos + 2] = arr[width * height * 4 - width * 4 * (i + 1) + j * 4 + 2];
//				arr[width * height * 4 - width * 4 * (i + 1) + j * 4 + 0] = tmp[0];
//				arr[width * height * 4 - width * 4 * (i + 1) + j * 4 + 1] = tmp[1];
//				arr[width * height * 4 - width * 4 * (i + 1) + j * 4 + 2] = tmp[2];
//			}
//		}
//	}
//	// Converting 32bitmap into 24bitmap (deleting alpha chanel)
//	if (needToConvert32bitTo24bit) {
//		int alphaCnt = 0;
//		for (long long i = 3; i < width * height * 3; i += 3) {
//			arr[i + 0] = arr[i + 1 + alphaCnt];
//			arr[i + 1] = arr[i + 2 + alphaCnt];
//			arr[i + 2] = arr[i + 3 + alphaCnt];
//			alphaCnt++;
//		}
//	}
//
//	// Writing pixels
//	bmp.write((char*)arr, width * height * (bitsPerPixel / 8));
//
//	delete[] arr;
//	bmp.close();
//
//	return 1;
//}