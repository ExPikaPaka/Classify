#pragma once
#include <string>
#include "..\dataTypes.h"

class Line {
	ui8v3* pixels = nullptr;
	ui32 width = 0;
public:
	Line() {};
	Line(ui32 width) {
		createLine(width);
	};
	~Line() {
		if (pixels)
			delete[] pixels;
	}
	
	void createLine(ui32 width) {
		this->width = width;
		pixels = new ui8v3[width];
	}

	ui8v3& operator[](ui32 index) {
		return pixels[index];
	}
};


class BMP {
public:
	Line *lines;

	int fileSize;
	int width;
	int height;
	std::string filePath;

	BMP();
	~BMP();
	bool read(const char* bmpFileName);
	bool create(const char* bmpFileName, long long bmpWidth, long long bmpHeight, unsigned char* source);

	Line& operator[](int index) {
		return lines[index];
	}
};

