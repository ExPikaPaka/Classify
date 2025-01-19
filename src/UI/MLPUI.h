#pragma once
#include "../dataTypes.h"
#include "IMGUI_UI.h"
#include "GameWindow.h"
#include "../Algorithm/entMath.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "../3dparty/stb_image/stb_image.h"



#include "algorithm"
#include "string"


ui32 createTexture(ui8* data, ui32 width, ui32 height, ui32 nChanels) {
	if (!data) {
		return 0;
	}

	ui32 textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	ui32 mode = nChanels == 3 ? GL_RGB : GL_RGBA;

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, mode, width, height, 0, mode, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	return textureID;
}


void deleteTexture(ui32 textureID) {
	glDeleteTextures(1, &textureID);
}


// Function to draw a rectangle with fill and optional outline
void drawRectangle(ImVec2 position, ImVec2 size, ui32 fillColor, ui32 outlineColor = 0, float outlineThickness = 0.0f, bool alwaysOnTop = false) {
	// Get the current window's draw list
	ImDrawList* draw_list = alwaysOnTop ? ImGui::GetForegroundDrawList() : ImGui::GetWindowDrawList();

	ImVec2 rect_min = position;											  // Top-left corner
	ImVec2 rect_max = ImVec2(position.x + size.x, position.y + size.y);	  // Bottom-right corner

	// Draw the filled rectangle
	draw_list->AddRectFilled(rect_min, rect_max, fillColor);

	// Draw the outline if outlineColor is not 0 and outlineThickness is greater than 0
	if (outlineColor != 0 && outlineThickness > 0.0f) {
		draw_list->AddRect(rect_min, rect_max, outlineColor, 0.0f, 0, outlineThickness);
	}
}


void fitImageIntoRectangle(ui32v2 imageSize, ui32v2 rectSize, ui32v2* outPosition, ui32v2* outSize) {
	float aspectRatio = (float)imageSize.x / (float)imageSize.y;
	float scaleFactor = std::min((float)rectSize.x / imageSize.x, (float)rectSize.y / imageSize.y);
	float scaledWidth = imageSize.x * scaleFactor;
	float scaledHeight = imageSize.y * scaleFactor;

	// Image position at the center + maximizing size ensuring aspect ratio
	*outPosition = ui32v2((rectSize.x - scaledWidth) / 2, (rectSize.y - scaledHeight) / 2);
	*outSize = ui32v2(scaledWidth, scaledHeight);
}


void renderInputImageWindow(ImVec2 position, ImVec2 size, ui32 imageTextureId, ui32 imageWidth, ui32 imageHeight, std::string windowName) {
	// Creating window
	ImGui::SetNextWindowPos(position);
	ImGui::SetNextWindowSize(size);
	ImGui::Begin(windowName.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

	// Rendering Image + data
	if (imageTextureId) {
		ui32v2 imgPos;
		ui32v2 imgSize;

		fitImageIntoRectangle({ imageWidth, imageHeight }, { size.x, size.y }, &imgPos, &imgSize);

		// Rendering image
		ImGui::SetCursorPos(ImVec2(imgPos.x, imgPos.y));
		ImGui::Image((void*)imageTextureId, ImVec2(imgSize.x, imgSize.y));


		// Displaying image size in pixels 
		std::string imgSizeText = std::to_string(imageWidth) + "x" + std::to_string(imageHeight);
		ImVec2 textSize = ImGui::CalcTextSize(imgSizeText.c_str());
		ImVec2 renderTextPosition = ImVec2(size.x - textSize.x - 5, 5);
		ImVec2 renderRectanglePosition = ImVec2(size.x - textSize.x - 10, position.y);
		ImVec2 rectanglePos = ImVec2(renderTextPosition.x - 5, renderTextPosition.y - 5);
		ImVec2 rectangleSize = ImVec2(textSize.x + 10, textSize.y + 10);
		ui32 fillColor = IM_COL32(0, 0, 0, 192);

		// Rendering background rectangle
		drawRectangle(renderRectanglePosition, rectangleSize, fillColor);

		// Rendering text
		ImGui::SetCursorPos(renderTextPosition);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		ImGui::Text("%s", imgSizeText.c_str());
		ImGui::PopStyleColor();

	} else { // Rendering "No image" text
		const char* text = "No image";
		ImVec2 textSize = ImGui::CalcTextSize(text);
		ImVec2 textPos = ImVec2((size.x - textSize.x) / 2, (size.y - textSize.y) / 2);
		ImGui::SetCursorPos(textPos);
		ImGui::Text("%s", text);
	}

	ImGui::End();
}

void renderSelectionRectangle(f32v2 startPos, f32v2 minPosition, f32v2 maxPosition) {
	ImGuiIO& io = ImGui::GetIO();
	ImVec2 mousePos = io.MousePos;

	startPos.x = startPos.x < minPosition.x ? minPosition.x : startPos.x;
	startPos.y = startPos.y < minPosition.y ? minPosition.y : startPos.y;

	mousePos.x = mousePos.x < minPosition.x ? minPosition.x : mousePos.x;
	mousePos.y = mousePos.y < minPosition.y ? minPosition.y : mousePos.y;

	startPos.x = startPos.x > maxPosition.x ? maxPosition.x : startPos.x;
	startPos.y = startPos.y > maxPosition.y ? maxPosition.y : startPos.y;

	mousePos.x = mousePos.x > maxPosition.x ? maxPosition.x : mousePos.x;
	mousePos.y = mousePos.y > maxPosition.y ? maxPosition.y : mousePos.y;

	ImVec2 size = ImVec2(mousePos.x - startPos.x, mousePos.y - startPos.y);
	ImVec2 rectStartPos = ImVec2(startPos.x, startPos.y);

	if (size.x == 0 || size.y == 0) return;

	// Draw the selection rectangle
	drawRectangle(rectStartPos, size, IM_COL32(87, 255, 218, 128), IM_COL32(87, 255, 218, 255), 1, true);
}


void cropImage(ui32v2& relStartPos, ui32v2& relEndPos, i32& nChanels, i32& imgWidth, i32& imgHeight, ui8** imageOriginal, ui8** imageCopy, ui32& imgTexId, ui32& imgCopyTexId, bool& needToUpdate) {
	// Check if selection region is at least 1x1 size
	bool needToCrop = false;
	if ((relEndPos.x - relStartPos.x) && (relEndPos.y - relStartPos.y)) {
		needToCrop = true;
	}

	printf_s("Inside calling cropImage:\n");
	printf("Pointer %p\n", *imageOriginal);

	// Cropping
	if (needToCrop) {
		ui32 newImageWidth = relEndPos.x - relStartPos.x;
		ui32 newImageHeight = relEndPos.y - relStartPos.y;

		printf_s("Dimen   : (%d %d)\n", newImageWidth, newImageHeight);

		// Copying segments
		ui8* newImage = new ui8[newImageWidth * newImageHeight * nChanels];
		printf("Acceesing in for\n");
		for (ui32 y = relStartPos.y; y < relEndPos.y; y++) {
			for (ui32 x = relStartPos.x; x < relEndPos.x; x++) {
				ui32 indexOld = y * imgWidth * nChanels + x * nChanels;
				ui32 indexNew = (y - relStartPos.y) * newImageWidth * nChanels + (x - relStartPos.x) * nChanels;

				newImage[indexNew + 0] = (*imageOriginal)[indexOld + 0];
				newImage[indexNew + 1] = (*imageOriginal)[indexOld + 1];
				newImage[indexNew + 2] = (*imageOriginal)[indexOld + 2];
				if (nChanels == 4) {
					newImage[indexNew + 3] = 255;
				}
			}
		}
		printf("Accesing end\n");

		// Replacing image
		imgWidth = newImageWidth;
		imgHeight = newImageHeight;

		deleteTexture(imgTexId);
		deleteTexture(imgCopyTexId);

		printf_s("Memory\n");
		if (*imageOriginal) {
			delete[] *imageOriginal;
			*imageOriginal = 0;
		}
		if (*imageCopy) {
			delete[] *imageCopy;
			*imageCopy = 0;
		}

		*imageOriginal = newImage;
		*imageCopy = new ui8[imgWidth * imgHeight * nChanels];
		imgCopyTexId = createTexture(*imageOriginal, imgWidth, imgHeight, nChanels);
		imgTexId = createTexture(*imageOriginal, imgWidth, imgHeight, nChanels);

		needToUpdate = true;
	}
}


void handleSelection(bool& needToUpdate, i32& imgWidth, i32& imgHeight, ui32& inputImage_WindowWidth, ui32& inputImage_WindowHeight,
						ui32& imgTexId, ui32& imgCopyTexId, bool& isSelecting, ui32v2& startPos, i32& nChanels, ui8** imageOriginal, ui8** imageCopy, ImVec2& size) {
	//// Selection //

	ImGuiIO& io = ImGui::GetIO();

	ui32v2 imgPos;
	ui32v2 imgSize;
	ui32v2 imgEndPos;

	fitImageIntoRectangle({ imgWidth, imgHeight }, { inputImage_WindowWidth, inputImage_WindowHeight }, &imgPos, &imgSize);
	imgEndPos = imgPos + imgSize;


	// Handle mouse input
	if (io.MouseDown[0] && imgTexId) { // LMB down
		if (!isSelecting) {
			ImVec2 mousePos = ImGui::GetMousePos();
			startPos = { mousePos.x, mousePos.y };
			startPos.x = startPos.x < imgPos.x ? imgPos.x : startPos.x;
			startPos.y = startPos.y < imgPos.y ? imgPos.y : startPos.y;
			isSelecting = true;
		}

		f32v2 minPos = { std::min(startPos.x, imgPos.x), std::min(startPos.y, imgPos.y) };
		f32v2 maxPos = { std::min(imgEndPos.x, inputImage_WindowWidth), std::min((f32)imgEndPos.y, inputImage_WindowHeight + size.y) };
		renderSelectionRectangle(startPos, minPos, maxPos);
	} else { // LMB up
		if (isSelecting) {
			if (io.MouseDown[1]) { // Canceling via RButton
				isSelecting = false;
			} else {

				ImVec2 endPos = ImGui::GetMousePos();

				// Image on screen corner positions
				ui32v2 imgTopLeft = imgPos;
				ui32v2 imgBottomRight = imgPos + imgSize;

				// Calculate endPosition from 0 to imageOnScreen location
				endPos.x = ent::math::clampValue(endPos.x, imgTopLeft.x, imgBottomRight.x) - imgTopLeft.x;
				endPos.y = ent::math::clampValue(endPos.y, imgTopLeft.y, imgBottomRight.y) - imgTopLeft.y;

				// Adjust startPos to be in range 0 to imageOnScreen location
				startPos.x -= imgTopLeft.x;
				startPos.y -= imgTopLeft.y;

				// Calculating relative position. This value represents mouse position on image source
				ui32v2 relStartPos = { ent::math::clampValue(((f32)startPos.x / (f32)imgSize.x) * (f32)imgWidth, 0, imgWidth),
										ent::math::clampValue(((f32)startPos.y / (f32)imgSize.y) * (f32)imgHeight, 0, imgHeight) };

				ui32v2 relEndPos = { ent::math::clampValue(((f32)endPos.x / (f32)imgSize.x) * (f32)imgWidth, 0, imgWidth),
										ent::math::clampValue(((f32)endPos.y / (f32)imgSize.y) * (f32)imgHeight, 0, imgHeight) };


				// Swapping start and end in case user dragged from bottom left to top right
				if (relStartPos.x > relEndPos.x) {
					std::swap(relStartPos.x, relEndPos.x);
				}
				if (relStartPos.y > relEndPos.y) {
					std::swap(relStartPos.y, relEndPos.y);
				}

				cropImage(relStartPos, relEndPos, nChanels, imgWidth, imgHeight, imageOriginal, imageCopy, imgTexId, imgCopyTexId, needToUpdate);

				isSelecting = false;
			}
		}
	}
}


void handleUI(bool& fitImage, bool& saveToFile, bool& classify, bool& clearFolder, bool& printClassifyToConsole, ui32& inputImage_WindowWidth, ui32& inputImage_WindowHeight, ent::ui::GameWindow& window, std::string& imageFilePath, bool& btOpenImagePressed, ui32& imgTexId, ui32& imgCopyTexId, i32& imgWidth, i32& imgHeight, i32& nChanels, ui8** imageOriginal, ui8** imageCopy, bool& needToUpdate, ent::util::Logger** logger, i32& brightnessBias, i32& contrastBias, bool& displayMask, bool& countValuesEqOrHigThanThreshold, bool& countValuesEqOrLowThanThreshold, bool& invertImageColors, bool& perChannelContrast, bool& blackAndWhite, bool** checkboxStates, ui32& totalClasses, ui32& totalCountedPixels, i32& horizontalSectorsCount, i32& verticalSectorsCount, bool& displaySectors, i32& countThreshold) {
	// Image input window
	inputImage_WindowWidth = window.getWidth() / 2;
	inputImage_WindowHeight = window.getHeight() / 2;
	ImVec2 size(inputImage_WindowWidth, inputImage_WindowHeight);


	ImGui::SetNextWindowPos(ImVec2(inputImage_WindowWidth, 10));
	ImGui::SetNextWindowSize(ImVec2(inputImage_WindowWidth - 10, window.getHeight() - 10));
	ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);

	ImGui::PushFont(0);

	ImGui::PushItemWidth(inputImage_WindowWidth - 30);
	ImGui::InputText(" ", (char*)imageFilePath.data(), imageFilePath.size());

	btOpenImagePressed = ImGui::Button("OPEN IMAGE", ImVec2(inputImage_WindowWidth - 30, 50));


	if (btOpenImagePressed) {
		ui8* loadedImage = stbi_load(imageFilePath.c_str(), &imgWidth, &imgHeight, &nChanels, 0);
		if (loadedImage) {
			deleteTexture(imgTexId);
			deleteTexture(imgCopyTexId);

			if (*imageCopy) {
				delete[] *imageCopy;
				*imageCopy = 0;
			}

			if (*imageOriginal) {
				delete[] *imageOriginal;
				*imageOriginal = 0;
			}

			*imageOriginal = loadedImage;

			*imageCopy = new ui8[imgWidth * imgHeight * nChanels];
			imgCopyTexId = createTexture(*imageOriginal, imgWidth, imgHeight, nChanels);
			imgTexId = createTexture(*imageOriginal, imgWidth, imgHeight, nChanels);
			needToUpdate = true;
		} else {
			(*logger)->addLog("Failed to load image: " + std::string(stbi_failure_reason()), ent::util::level::W);
		}
	}

	if (ImGui::CollapsingHeader("Settings")) {
		if (ImGui::Button("FIT IMAGE", ImVec2(inputImage_WindowWidth - 30, 50))) {
			if (totalCountedPixels) {
				fitImage = true;
				needToUpdate = true;
			}
		}

		ImGui::Dummy(ImVec2(0, 10)); // Spacing

		ImGui::Text("Brightness");
		if (ImGui::SliderInt("##Brightness", &brightnessBias, -255, 255)) {
			needToUpdate = true;
		}

		ImGui::Dummy(ImVec2(0, 5)); // Spacing


		ImGui::Text("Contrast");
		if (ImGui::SliderInt("##Contrast", &contrastBias, -127, 127)) {
			needToUpdate = true;
		}

		ImGui::Dummy(ImVec2(0, 10)); // Spacing


		if (ImGui::Checkbox("Black and white", &blackAndWhite)) {
			needToUpdate = true;
		}

		if (ImGui::Checkbox("Per channel contrast", &perChannelContrast)) {
			needToUpdate = true;
		}

		if (ImGui::Checkbox("Invert colors", &invertImageColors)) {
			needToUpdate = true;
		}

		ImGui::Dummy(ImVec2(0, 10)); // Spacing


		if (ImGui::Checkbox("Count pixels <= than threshold", &countValuesEqOrLowThanThreshold)) {
			needToUpdate = true;
		}

		if (ImGui::Checkbox("Count pixels >= than threshold", &countValuesEqOrHigThanThreshold)) {
			countValuesEqOrLowThanThreshold = false;
			needToUpdate = true;
		}

		if (ImGui::Checkbox("Display threshold", &displayMask)) {
			needToUpdate = true;
		}

		if (ImGui::Checkbox("Print to console", &printClassifyToConsole)) {
			needToUpdate = true;
		}

		if (countValuesEqOrLowThanThreshold) {
			countValuesEqOrHigThanThreshold = false;
		}

		if (countValuesEqOrHigThanThreshold) {
			countValuesEqOrLowThanThreshold = false;
		}

		if (ImGui::SliderInt("##Threshold", &countThreshold, 0, 255)) {
			needToUpdate = true;
		}

		ImGui::Dummy(ImVec2(0, 10)); // Spacing


		if (ImGui::Checkbox("Display sectors", &displaySectors)) {
			needToUpdate = true;
		}

		if (ImGui::SliderInt("##Vertical sectors count", &verticalSectorsCount, 1, 16)) {
			needToUpdate = true;
		}

		if (ImGui::SliderInt("##Horizontal sectors count", &horizontalSectorsCount, 1, 16)) {
			needToUpdate = true;
		}

		ImGui::Dummy(ImVec2(0, 10)); // Spacing

		std::string countedPixelsString = "Counted pixels: " + std::to_string(totalCountedPixels);
		ImGui::Text(countedPixelsString.c_str());


		ImGui::Dummy(ImVec2(0, 10)); // Spacing

		// Create 16 horizontal checkboxes labeled A-F
		for (int i = 0; i < totalClasses; i++) {
			ImGui::Checkbox((std::string("##chbk") + std::to_string(i)).c_str(), &(*checkboxStates)[i]);
			ImGui::SameLine(0, 16);
		}

		ImGui::NewLine();
		ImGui::Text("  0      1      2      3      4      5      6      7      8      9");

		ImGui::Dummy(ImVec2(0, 10)); // Spacing

		saveToFile = ImGui::Button("SAVE", ImVec2((inputImage_WindowWidth - 30) / 2 - 2, 50));
		ImGui::SameLine();
		classify = ImGui::Button("CLASSIFY", ImVec2((inputImage_WindowWidth - 30) / 2 - 2, 50));


		clearFolder = ImGui::Button("CLEAR FOLDER", ImVec2((inputImage_WindowWidth - 30), 30));
	}


	const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK", "LLLLLLL", "MMMM", "OOOOOOO", "PPPP", "QQQQQQQQQQ", "RRR", "SSSS" };
	static const char* current_item = items[0];

	if (ImGui::BeginCombo("##combo", current_item)) // The second parameter is the label previewed before opening the combo.
	{
		for (int n = 0; n < 14; n++) {
			bool is_selected = (current_item == items[n]); // You can store your selection however you want, outside or inside your objects
			if (ImGui::Selectable(items[n], is_selected)) {
				current_item = items[n];
				//if (is_selected)
					//ImGui::SetItemDefaultFocus();   // You may set the initial focus when opening the combo (scrolling + for keyboard navigation support)
			}
		}
			ImGui::EndCombo();
	}


	ImGui::PopFont();
	ImGui::End();
}