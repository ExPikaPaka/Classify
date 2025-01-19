//#include "dataTypes.h"
//#include <iostream>
//#include "Image/BMP.h"
//#include "Image/BMP.h"
//#include "UI/GameWindow.h"
//#include "UI/IMGUI_UI.h"
//#include "UI/Text.h"
//#include "UI/Image.h"
//#include "UI/MLPUI.h"
//
//#include "Algorithm/entMath.h"
//
//#define STB_IMAGE_IMPLEMENTATION
//#include "3dparty/stb_image/stb_image.h"
//
//#include <iomanip>
//
//#include "IO/FileOperations.h"
//#include "Algorithm/entString.h"
//
//#include <sstream>
//
//
//
//int maain() {
//	// Window initialization
//	ent::ui::GameWindow window;
//	window.setWindowSize(900, 900);
//	window.init();
//	window.setBlend(true);
//
//	// Logger initialization
//	ent::util::Logger* logger = ent::util::Logger::getInstance();
//
//	// Imgui initialization
//	ent::ui::imgui::init((GLFWwindow*)window.getHandle());
//	ent::ui::imgui::SetupImGuiStyle();
//
//	std::string minecraftFontPath = "res/fonts/Minecraft.ttf";
//	std::ifstream ftMinecraftFile(minecraftFontPath);
//
//	if (ftMinecraftFile.good()) {
//		ImGuiIO& io = ImGui::GetIO();
//		io.Fonts->AddFontFromFileTTF(minecraftFontPath.c_str(), 18);
//		io.Fonts->Build();
//	} else {
//		logger->addLog("Failed to load font" + minecraftFontPath, ent::util::level::F);
//		return -1;
//	}
//
//
//
//
//	// Image input
//	i32 imgWidth = 0;
//	i32 imgHeight = 0;
//	i32 nChanels = 0;
//	i32 contrastBias = 0;
//	i32 brightnessBias = 0;
//
//	bool btOpenImagePressed = false;
//	bool needToUpdate = false;
//
//	std::string imageFilePath;
//	imageFilePath.resize(4096);
//
//	bool isSelecting = false;
//	ui32v2 startPos;
//
//
//	ui32 imgTexId = 0;
//	ui32 imgCopyTexId = 0;
//	ui8* imageOriginal = nullptr;
//	ui8* imageCopy = nullptr;
//
//	ui32 inputImage_WindowWidth = 0;
//	ui32 inputImage_WindowHeight = 0;
//
//	bool blackAndWhite = false;
//	bool perChannelContrast = false;
//	bool invertImageColors = false;
//	bool displaySectors = false;
//	bool displayMask = false;
//	bool printClassifyToConsole = false;
//	bool saveToFile = false;
//	bool classify = false;
//	bool clearFolder = false;
//	bool fitImage = false;
//
//	bool countValuesEqOrLowThanThreshold = true;
//	bool countValuesEqOrHigThanThreshold = false;
//	i32 countThreshold = 0;
//
//	ui32 totalCountedPixels = 0;
//
//	i32 horizontalSectorsCount = 9;
//	i32 verticalSectorsCount = 9;
//	i32* countedPixelsPerSector = new i32[horizontalSectorsCount * verticalSectorsCount];
//
//	std::string classifyPath = "res\\classify\\";
//
//	ui32 totalClasses = 10;
//	bool* checkboxStates = new bool[totalClasses];
//	f32* classifyMatch = new f32[totalClasses];
//	std::memset(checkboxStates, 0, sizeof(bool) * totalClasses);
//	std::memset(classifyMatch, 0, sizeof(f32) * totalClasses);
//
//	while (!glfwWindowShouldClose((GLFWwindow*)window.getHandle())) {
//		//printf_s("%x\n", (void*)imageOriginal);
//		// Handle events /////////////////////////////////////////////////////////////////////
//		glfwPollEvents();
//		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//		//////////////////////////////////////////////////////////////////////////////////////
//
//		// Update & Render ////////////////////////////////////////////////////////////////////////////
//		ent::ui::imgui::newFrame();
//
//		inputImage_WindowWidth = window.getWidth() / 2;
//		inputImage_WindowHeight = window.getHeight() / 2;
//		ImVec2 size(inputImage_WindowWidth, inputImage_WindowHeight);
//
//
//		// Main UI
//		handleUI(fitImage, saveToFile, classify, clearFolder, printClassifyToConsole, inputImage_WindowWidth, inputImage_WindowHeight, window, imageFilePath, btOpenImagePressed, imgTexId, imgCopyTexId, imgWidth, imgHeight, nChanels, &imageOriginal, &imageCopy, needToUpdate, &logger, brightnessBias, contrastBias, displayMask, countValuesEqOrHigThanThreshold, countValuesEqOrLowThanThreshold, invertImageColors, perChannelContrast, blackAndWhite, &checkboxStates, totalClasses, totalCountedPixels, horizontalSectorsCount, verticalSectorsCount, displaySectors, countThreshold);
//		handleSelection(needToUpdate, imgWidth, imgHeight, inputImage_WindowWidth, inputImage_WindowHeight, imgTexId, imgCopyTexId, isSelecting, startPos, nChanels, &imageOriginal, &imageCopy, size);
//		renderInputImageWindow(ImVec2(0, 0), size, imgTexId, imgWidth, imgHeight, "Image Preview");
//
//		if (fitImage) {
//			ui32 minX = imgWidth;
//			ui32 maxX = 0;
//			ui32 minY = imgHeight;
//			ui32 maxY = 0;
//
//			for (int y = 0; y < imgHeight; y++) {
//				for (int x = 0; x < imgWidth; x++) {
//					ui32 rPos = y * imgWidth * nChanels + x * nChanels;
//					ui8v3 p = { imageCopy[rPos + 0], imageCopy[rPos + 1], imageCopy[rPos + 2] };
//					f32 avg = (p.r + p.g + p.b) / 3.0f;
//
//					if (countValuesEqOrLowThanThreshold && avg <= countThreshold) {
//						if (x < minX) minX = x;
//						if (x > maxX) maxX = x;
//
//						if (y < minY) minY = y;
//						if (y > maxY) maxY = y;
//					}
//
//					if (countValuesEqOrHigThanThreshold && avg >= countThreshold) {
//						if (x < minX) minX = x;
//						if (x > maxX) maxX = x;
//
//						if (y < minY) minY = y;
//						if (y > maxY) maxY = y;
//					}
//				}
//			}
//
//			f32 padding = 0.1;
//			ui32v2 relStartPos = { minX,  minY };
//			ui32v2 relEndPos = { maxX + 1,  maxY + 1 };
//
//			ui32 newWidth = relEndPos.x - relStartPos.x;
//			ui32 newHeight = relEndPos.y - relStartPos.y;
//
//			// Rectangle shape
//			if (newWidth > newHeight) {
//				ui32 midPos = newHeight / 2;
//				relStartPos.y = (relStartPos.y + midPos) - newWidth / 2;
//				relEndPos.y = (relEndPos.y - midPos) + newWidth / 2;
//			} else {
//				ui32 midPos = newWidth / 2;
//				relStartPos.x = (relStartPos.x + midPos) - newHeight / 2;
//				relEndPos.x = (relEndPos.x - midPos) + newHeight / 2;
//			}
//
//			ui32 dimension = newWidth > newHeight ? newWidth : newHeight;
//			// Add padding
//			relStartPos.x = ent::math::clampValue(relStartPos.x - padding * dimension, 0, imgWidth);
//			relStartPos.y = ent::math::clampValue(relStartPos.y - padding * dimension, 0, imgHeight);
//			relEndPos.x = ent::math::clampValue(relEndPos.x + padding * dimension, 0, imgWidth);
//			relEndPos.y = ent::math::clampValue(relEndPos.y + padding * dimension, 0, imgHeight);
//
//
//
//			printf_s("relStart: (%d %d)\n", relStartPos.x, relStartPos.y);
//			printf_s("relEnd  : (%d %d)\n", relEndPos.x, relEndPos.y);
//			printf_s("Dimen   : (%d %d)\n", imgWidth, imgHeight);
//			printf_s("Pointer %p\n", imageOriginal);
//			cropImage(relStartPos, relEndPos, nChanels, imgWidth, imgHeight, &imageOriginal, &imageCopy, imgTexId, imgCopyTexId, needToUpdate);
//
//			fitImage = false;
//			needToUpdate = true;
//		}
//
//		if (needToUpdate) {
//			printf_s("\nGlobal %p\n", (void*)imageOriginal);
//		}
//
//
//
//		if (imgTexId && needToUpdate) {
//			delete[] countedPixelsPerSector;
//			countedPixelsPerSector = new i32[horizontalSectorsCount * verticalSectorsCount];
//			std::memset(countedPixelsPerSector, 0, verticalSectorsCount * horizontalSectorsCount * sizeof(i32));
//			totalCountedPixels = 0;
//			for (int y = 0; y < imgHeight; y++) {
//				for (int x = 0; x < imgWidth; x++) {
//					ui32 rPos = y * imgWidth * nChanels + x * nChanels;
//
//					ui8v3 p = { imageOriginal[rPos + 0], imageOriginal[rPos + 1], imageOriginal[rPos + 2] };
//
//					// Black and white
//					f32 avg = (p.r + p.g + p.b) / 3.0;
//					if (blackAndWhite) {
//						p = { avg, avg, avg };
//					}
//
//					// Brightness adjustment
//					brightnessBias = invertImageColors ? -brightnessBias : brightnessBias;
//					p.r = ent::math::clampValue(p.r + brightnessBias, 0, 255);
//					p.g = ent::math::clampValue(p.g + brightnessBias, 0, 255);
//					p.b = ent::math::clampValue(p.b + brightnessBias, 0, 255);
//					brightnessBias = invertImageColors ? -brightnessBias : brightnessBias;
//
//					// Contrast adjustment
//					f32 f = contrastBias;
//					if (f > 0) { // Increase contrast
//						f32 v = (p.r + p.g + p.b) / 3.0;
//						p.r = (perChannelContrast ? v : p.r) >= 128 ? ent::math::mapValue(p.r, 128, 255, 128 + f, 255) : ent::math::mapValue(p.r, 0, 127, 0, 127 - f);
//						p.g = (perChannelContrast ? v : p.g) >= 128 ? ent::math::mapValue(p.g, 128, 255, 128 + f, 255) : ent::math::mapValue(p.g, 0, 127, 0, 127 - f);
//						p.b = (perChannelContrast ? v : p.b) >= 128 ? ent::math::mapValue(p.b, 128, 255, 128 + f, 255) : ent::math::mapValue(p.b, 0, 127, 0, 127 - f);
//
//					} else { // Decrease Contrast
//						f32 v = (p.r + p.g + p.b) / 3.0;
//						p.r = (perChannelContrast ? v : p.r) >= 128 ? ent::math::mapValue(p.r, 128, 255, 128, 255 + f) : ent::math::mapValue(p.r, 0, 127, 0 - f, 127);
//						p.g = (perChannelContrast ? v : p.g) >= 128 ? ent::math::mapValue(p.g, 128, 255, 128, 255 + f) : ent::math::mapValue(p.g, 0, 127, 0 - f, 127);
//						p.b = (perChannelContrast ? v : p.b) >= 128 ? ent::math::mapValue(p.b, 128, 255, 128, 255 + f) : ent::math::mapValue(p.b, 0, 127, 0 - f, 127);
//					}
//
//					// Inverting colors;
//					if (invertImageColors) {
//						p.r = 255 - p.r;
//						p.g = 255 - p.g;
//						p.b = 255 - p.b;
//					}
//
//					// Calculate the sector the pixel belongs to
//					i32 sectorX = (x * verticalSectorsCount) / imgWidth;
//					i32 sectorY = (y * horizontalSectorsCount) / imgHeight;
//					i32 sectorIndex = sectorY * verticalSectorsCount + sectorX;
//
//					// Count based on the threshold
//					avg = (p.r + p.g + p.b) / 3.0f;
//					imageCopy[rPos + 0] = p.r;
//					imageCopy[rPos + 1] = p.g;
//					imageCopy[rPos + 2] = p.b;
//
//					if (countValuesEqOrLowThanThreshold && avg <= countThreshold) {
//						countedPixelsPerSector[sectorIndex]++;
//						totalCountedPixels++;
//
//						if (displayMask) {
//							imageCopy[rPos + 0] = (x + y % 2) % 2 ? 255 : 0;
//							imageCopy[rPos + 1] = 0;
//							imageCopy[rPos + 2] = (x + y % 2) % 2 ? 255 : 0;
//						}
//					}
//
//					if (countValuesEqOrHigThanThreshold && avg >= countThreshold) {
//						countedPixelsPerSector[sectorIndex]++;
//						totalCountedPixels++;
//
//						if (displayMask) {
//							imageCopy[rPos + 0] = (x + y % 2) % 2 ? 255 : 0;
//							imageCopy[rPos + 1] = 0;
//							imageCopy[rPos + 2] = (x + y % 2) % 2 ? 255 : 0;
//						}
//					}
//
//
//
//
//					if (nChanels == 4) {
//						imageCopy[rPos + 3] = 255;
//					}
//				}
//			}
//
//			// Draw sectors
//			if (displaySectors) {
//				for (int y = 0; y < imgHeight; y += imgHeight / horizontalSectorsCount) {
//					for (int x = 0; x < imgWidth; x++) {
//						ui32 rPos = y * imgWidth * nChanels + x * nChanels;
//
//						imageCopy[rPos + 0] = 255;
//						imageCopy[rPos + 1] = 0;
//						imageCopy[rPos + 2] = 0;
//					}
//				}
//				for (int x = 0; x < imgWidth; x += imgWidth / verticalSectorsCount) {
//					for (int y = 0; y < imgHeight; y++) {
//						ui32 rPos = y * imgWidth * nChanels + x * nChanels;
//
//						imageCopy[rPos + 0] = 255;
//						imageCopy[rPos + 1] = 0;
//						imageCopy[rPos + 2] = 0;
//					}
//				}
//			}
//
//			// Print vector
//			if (printClassifyToConsole) {
//				std::cout << "\n\nTotal counted pixels: " << totalCountedPixels << "\n";
//				for (int y = 0; y < horizontalSectorsCount; y++) {
//					for (int x = 0; x < verticalSectorsCount; x++) {
//						i32 sectorIndex = y * verticalSectorsCount + x;
//						std::cout << "Sector [" << y << "][" << x << "]: " << countedPixelsPerSector[sectorIndex] << "\n";
//					}
//				}
//
//				// Print normalized vector
//				f32 total = totalCountedPixels;
//				std::cout << "Normalized vector: [";
//				for (int i = 0; i < horizontalSectorsCount * verticalSectorsCount; i++) {
//					if (total) {
//						std::cout << (f32)countedPixelsPerSector[i] / total << (i == (horizontalSectorsCount * verticalSectorsCount - 1) ? "" : ", ");
//
//					} else {
//						std::cout << "0" << (i == (horizontalSectorsCount * verticalSectorsCount - 1) ? "" : ", ");
//					}
//				}
//				std::cout << "]";
//			}
//
//			deleteTexture(imgCopyTexId);
//			imgCopyTexId = createTexture(imageCopy, imgWidth, imgHeight, nChanels);
//			needToUpdate = false;
//		}
//
//		if (saveToFile && totalCountedPixels) {
//			std::ofstream file;
//			ent::io::createDirectory(ent::algoright::stringToWstring(classifyPath));
//			for (ui32 classType = 0; classType < totalClasses; classType++) {
//				if (checkboxStates[classType]) { // If select some checkbox
//					file.open(classifyPath + std::to_string(classType), std::ios_base::app);
//					file << std::fixed << std::setprecision(6);
//
//					for (ui32 i = 0; i < horizontalSectorsCount * verticalSectorsCount; i++) {
//						if (i == 0) {
//							file << horizontalSectorsCount << " " << verticalSectorsCount << " ";
//						}
//						file << (f32)countedPixelsPerSector[i] / (f32)totalCountedPixels << " ";
//					}
//					file << "\n";
//					file.close();
//				}
//			}
//			saveToFile = false;
//		}
//
//		if (clearFolder) {
//			ent::io::deleteAllFilesInDirectory(ent::algoright::stringToWstring(classifyPath));
//		}
//
//		if (classify && totalCountedPixels) {
//			std::cout << "\n";
//			std::ifstream file;
//
//			f32 minDiff = 10000000;
//			ui32 minDiffID = 0;
//			for (ui32 classType = 0; classType < totalClasses; classType++) {
//				file.open(classifyPath + std::to_string(classType));
//
//				ui32 sourceHorSectorsCount = 0;
//				ui32 sourceVerSectorsCount = 0;
//				f32* pixelsPerImage = 0;
//				classifyMatch[classType] = 0;
//				ui32 compatibleClassesCnt = 0;
//
//				i32* countedPixelsPerSectorMin = 0;
//				i32* countedPixelsPerSectorMax = 0;
//
//				std::string line;
//				// Reading line by line
//				while (std::getline(file, line)) {
//					std::istringstream iss(line);
//
//					// Extracting dimensions
//					ui32 curHorSectorsCount = 0;
//					ui32 curVerSectorsCount = 0;
//					iss >> curHorSectorsCount;
//					iss >> curVerSectorsCount;
//
//					if (pixelsPerImage == 0) {
//						pixelsPerImage = new f32[curHorSectorsCount * curVerSectorsCount];
//						std::memset(pixelsPerImage, 0, sizeof(f32) * curHorSectorsCount * curVerSectorsCount);
//						sourceHorSectorsCount = curHorSectorsCount;
//						sourceVerSectorsCount = curVerSectorsCount;
//
//						countedPixelsPerSectorMin = new i32[curHorSectorsCount * curVerSectorsCount];
//						std::memset(countedPixelsPerSectorMin, 100000, curVerSectorsCount * curHorSectorsCount * sizeof(i32));
//
//						countedPixelsPerSectorMax = new i32[curHorSectorsCount * curVerSectorsCount];
//						std::memset(countedPixelsPerSectorMax, 0, curVerSectorsCount * curHorSectorsCount * sizeof(i32));
//					}
//
//					if (curHorSectorsCount != sourceHorSectorsCount || curVerSectorsCount != sourceVerSectorsCount) {
//						continue; // Skip in case wrong dimensions
//					}
//
//					compatibleClassesCnt++;
//
//					// Extracting vector
//					for (ui32 i = 0; i < curHorSectorsCount * curVerSectorsCount; i++) {
//						f32 value;
//						iss >> value;
//						pixelsPerImage[i] += value;
//
//						if (value > countedPixelsPerSectorMax[i]) countedPixelsPerSectorMax[i] = value;
//						if (value < countedPixelsPerSectorMin[i]) countedPixelsPerSectorMin[i] = value;
//					}
//				}
//				// Average values
//				//std::cout << "\n\nAverage\n";
//				for (ui32 i = 0; i < sourceHorSectorsCount * sourceVerSectorsCount; i++) {
//					pixelsPerImage[i] /= compatibleClassesCnt;
//					//std::cout << pixelsPerImage[i] << " ";
//				}
//
//				// Calculating difference
//				ui32 diffSectorsCnt = 0;
//				f32 totalDiff = 0;
//				for (ui32 i = 0; i < sourceHorSectorsCount * sourceVerSectorsCount; i++) {
//					f32 valueA = pixelsPerImage[i];
//					f32 valueB = countedPixelsPerSector[i] / (f32)totalCountedPixels;
//					f32 diff = (1.0 - std::min(valueA, valueB) / std::max(valueA, valueB));
//
//					if (valueA == 0 || valueB == 0) diff = 1;
//					if (valueA == 0 && valueB == 0) diff = 0;
//					//std::cout << "Secotr " << i << " diff: " << diff << "\n";
//
//					totalDiff += diff;
//					diffSectorsCnt++;
//				}
//				totalDiff = totalDiff > 0 ? totalDiff / (sourceHorSectorsCount * sourceVerSectorsCount) : 10000000;
//				classifyMatch[classType] = totalDiff;
//				if (totalDiff < minDiff) {
//					minDiff = totalDiff;
//					minDiffID = classType;
//				}
//
//				file.close();
//			}
//
//
//			// Console print
//			for (ui32 classType = 0; classType < totalClasses; classType++) {
//				std::cout << "Class " << classType << " error = " << (classifyMatch[classType] == 10000000 ? "-----" : std::to_string(classifyMatch[classType])) << (classType == minDiffID ? " MIN\n" : "\n");
//			}
//
//			std::memset(checkboxStates, 0, sizeof(bool) * totalClasses);
//			checkboxStates[minDiffID] = 1;
//
//		}
//
//		//std::cout << "Rendering output\n";
//		renderInputImageWindow(ImVec2(0, size.y), size, imgCopyTexId, imgWidth, imgHeight, "Image Output");
//		///////////////////////
//
//
//
//		////////////////////
//
//
//		//std::cout << "MainUI\n\n";
//
//
//
//		ent::ui::imgui::renderFrame((GLFWwindow*)window.getHandle());
//
//
//
//		glfwSwapBuffers((GLFWwindow*)window.getHandle());
//		//////////////////////////////////////////////////////////////////////////////////////
//
//
//		// Update ////////////////////////////////////////////////////////////////////////////	
//
//
//		//////////////////////////////////////////////////////////////////////////////////////
//	}
//
//	return 0;
//}