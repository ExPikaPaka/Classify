#pragma once
#include "..\dataTypes.h"

class Image {
private:
	ui8v3 *pixels = nullptr;

	ui32 width = 0;
	ui32 height = 0;
public:

	Image() {};
	Image(ui32 width, ui32 height) {
		setSize(width, height);
	}
	~Image() {
		if (pixels != nullptr) {
			delete[] pixels;
		}
	}

	ui32 getWidth() const {
		return width;
	}

	ui32 getHeight() const {
		return height;
	}

	bool isInitialized() const {
		return initialized;
	}
	void setSize(ui32 width, ui32 height) {
		if (width == this->width && height == this->height) {
			return;
		}

		if (pixels != nullptr) {
			delete[] pixels;
		}

		this->width = width;
		this->height = height;

		if (width > 0 && height > 0) {
			pixels = new ui8v3[width * height];
			initialized = true;
		} else {
			pixels = nullptr;
			initialized = false;
		}
	}

	void setImage(const ui8v3* pixels, ui32 width, ui32 height) {
		if (this->width != width || height != this->height) {
			setSize(width, height);
		}

		for (ui32 i = 0; i < width * height; i++) {
			this->pixels[i] = pixels[i];
		}
	}

	struct Row {
		Row(ui8v3* data, ui32 width) {
			this->data = data;
			this->width = width;
		}

		ui8v3& operator[](ui32 col) {
			return data[col];
		}

	private:
		ui8v3* data;
		ui32 width;
	};

	Row operator[](ui32 row) {
		return Row(&pixels[row * width], width);
	}
private:
	bool initialized = false;
};