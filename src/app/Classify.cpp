#include "Classify.h"
#include "../UI/IMGUI_UI.h"
#include "../Algorithm/entMath.h"
#include "../IO/FileOperations.h"
#include "../Algorithm/entString.h"
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "../3dparty/stb_image/stb_image.h"


Classify::Classify() {
	// Window initialization
	m_window.setWindowSize(windowWidth, windowHeight);
	m_window.init();
	m_window.setBlend(true);

	// Logger initialization
	ent::util::Logger* logger = ent::util::Logger::getInstance();

	// Imgui initialization
	ent::ui::imgui::init((GLFWwindow*)m_window.getHandle());
	ent::ui::imgui::SetupImGuiStyle();

	// Font
	std::string fontPath = "res/fonts/Carlito-Bold.ttf";
	std::ifstream fontFile(fontPath);

	if (fontFile.good()) {
		ImGuiIO& io = ImGui::GetIO();
		static const ImWchar fullRange[] = { 0x0020, 0xFFFF, 0 }; // Unicode range from space to the end of BMP

		io.Fonts->AddFontFromFileTTF(fontPath.c_str(), fontSize, 0, fullRange);
		io.Fonts->Build();
	} else {
		logger->addLog("Failed to load font" + fontPath, ent::util::level::F);
	}

	// Member variables initialization
	m_classifyMethod = ClassifyMethod::minMax;
	m_maskType = MaskType::eqOrLower;	
	m_imagePath.resize(imagePathBufferSize);
	m_logger = ent::util::Logger::getInstance();
	m_imageSize = { 0, 0 };
	m_imageChannelsCnt = 0;
	m_imageOriginal = nullptr;
	m_imagePreview = nullptr;
	m_imageOriginalTextureID = 0;
	m_imagePreviewTextureID = 0;
	m_imageBrightness = 0;
	m_imageContrast = 0;
	m_imageBlackAndWhite = false;
	m_imageInvertColors = false;
	m_displayMaskThreshold = true;
	m_needToUpdate = false;
	m_maskThreshold = 0;
	m_displaySectors = true;
	m_sectorsType = SectorType::rectangular;
	m_horizontalSectorsCnt = 8;
	m_verticalSectorsCnt = 8;
	m_circleSectorsCnt = 8;
	if (m_sectorsType == SectorType::rectangular) {
		m_countedPixelsPerSector = new f32[m_horizontalSectorsCnt * m_verticalSectorsCnt];
	} else if (m_sectorsType == SectorType::circle) {
		m_countedPixelsPerSector = new f32[m_circleSectorsCnt];
	}
	m_totalSectorsCountedPixels = 0;
	m_classesCount = 10;
	m_classesCheckboxes = new bool[m_classesCount];
	std::memset(m_classesCheckboxes, 0, m_classesCount * sizeof(bool));
	m_mouseStartPos = { 0, 0 };
	m_mouseEndPos = { 0, 0 };
	m_isSelecting = false;
	m_needToLogToConsole = false;
	m_needToClassify = false;
	m_classifyFolderPath = "res/classify/";
	m_vectorNormalizationType = VectorNormalization::normal;
	m_needToSave = false;
	m_needToAutoCrop = false;
	m_autoCropPadding = 0.1;
	m_autoCropRectangular = true;
	m_autoCropFillColor = { 0.5, 0.5, 0.5, 1 };
	m_needToSelectAutoCropColor = false;
	m_circleOffset = 1.0;
	m_circleRadius = 20;
	m_CombineAngleThreshold = 0.1;
	m_circlePrecision = 180;
	m_maxIter = 20;
	m_lerpIter = 100;
	m_displayAngleAux = true;
	m_displayRectangles = false;
	m_anglesCount.resize(4, 0);
	m_imageAISize = { 28,28 };
	m_layersCount = 5;
	m_imageAI = 0;
	m_imageAIBW = 0;
	m_imageAITextureID = 0;
	m_neuronsPerLayer = new i32[m_layersCount];
	m_neuronsPerLayer[0] = m_imageAISize.x * m_imageAISize.y;
	for (int i = 1; i < m_layersCount - 1; i++) {
		m_neuronsPerLayer[i] = 40;
	}
	m_neuronsPerLayer[m_layersCount - 1] = m_classesCount;
	m_perceptron.init(m_layersCount, m_neuronsPerLayer);
	m_needToUpdateAI = false;
	m_needToTrainAI = false;
	m_batchSize = 1;
	m_aiAccuracyTarget = 0.8;
	m_learningRate = 0.01;
	m_perceptron.fillWeights();
	m_needToSaveWeights = false;
	m_weightsFileName = "perceptron";
	m_needToLoadWeights = false;
	m_aiError = 0;
}

void Classify::runLoop() {
	while (!glfwWindowShouldClose((GLFWwindow*)m_window.getHandle())) {
		handleUI();
		handleProcessing();
	}
}

void Classify::handleUI() {
	//////// Clear & handle events ////////////////////////////////////
	glfwPollEvents();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	ent::ui::imgui::newFrame();
	///////////////////////////////////////////////////////////////////

	///////// UI //////////////////////////////////////////////////////
	ImGui::PushFont(0);

	ui32 width = m_window.getWidth() / 2;
	ui32 height = m_window.getHeight();
	f32v4 borderColor = { 0.286, 0.30, 0.36, 1 };
	std::string borderPattern = "=";

	beginControlWindow({ m_window.getWidth() / 2 , 0 }, { width, height });

	// Image input/edit and method selection
	handleImageOpen();

	handleImageAutoCrop();
	imguiPadding(2, paddingMedium);

	handleImageProcessing();
	imguiPadding(0, paddingMedium);

	handleClassifyMethodSelection();
	std::string logToConsoleString = "Log to console";
	if (ImGui::Checkbox(logToConsoleString.c_str(), &m_needToLogToConsole)) { m_needToUpdate = true; }
	drawPattern(borderPattern, borderColor);

	switch (m_classifyMethod) {
		case ClassifyMethod::minMax:
			handleMinMaxMethod();
			break;
		case ClassifyMethod::averageDist:
			handleAverageDistMethod();
			break;
		case ClassifyMethod::angle:
			handleAngleMethod();
			break;
		case ClassifyMethod::perceptron:
			handlePerceptronMethod();
			break;
		case ClassifyMethod::genetic:
			handlePerceptronMethod();
			break;
		default:
			break;
	}
	endControlWindow();

	// Images
	ui32v2 imageWindowSize = { m_window.getWidth() / 2 - 4, m_window.getHeight() / 2 };
	drawImage({ 0,0 }, imageWindowSize, m_imageOriginalTextureID, m_imageSize);
	drawImage({ 0,imageWindowSize.y }, imageWindowSize, m_imagePreviewTextureID, m_imageSize);

	handleImageSelection();
	handleAutoCrop();


	ImGui::PopFont();
	///////////////////////////////////////////////////////////////////
	
	//////// Render ///////////////////////////////////////////////////
	ent::ui::imgui::renderFrame((GLFWwindow*)m_window.getHandle());
	selectAutoCropFillColor(); // This is done here to select actual color that is visible on screen
	glfwSwapBuffers((GLFWwindow*)m_window.getHandle());
	///////////////////////////////////////////////////////////////////
}

void Classify::beginControlWindow(i32v2 position, i32v2 shape) {
	ImGui::SetNextWindowPos(ImVec2(position.x, position.y));
	ImGui::SetNextWindowSize(ImVec2(shape.x, shape.y));
	ImGui::Begin("##controlWindow", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

}

void Classify::handleProcessing() {
	if ((m_classifyMethod == ClassifyMethod::perceptron && m_needToTrainAI) || m_classifyMethod == ClassifyMethod::genetic && m_needToTrainAI) {
		trainAI();
	}

	if (!m_needToUpdate || !m_imageOriginal || !m_imagePreview) return;
	std::memcpy(m_imagePreview, m_imageOriginal, m_imageSize.x * m_imageSize.y * m_imageChannelsCnt * sizeof(ui8));
	std::memset(m_imageMask, 0, m_imageSize.x * m_imageSize.y * m_imageChannelsCnt * sizeof(ui8));

	processImageOptions();

	// MinMax and AverageDistance method
	if (m_classifyMethod == ClassifyMethod::minMax || m_classifyMethod == ClassifyMethod::averageDist) {
		if (m_sectorsType == SectorType::rectangular) {
			processRectangularSectors();
			if (m_displaySectors) drawRectangularSectors();
		}
		if (m_sectorsType == SectorType::circle) {
			processCircleSectors();
		}
		if (m_needToLogToConsole) printToConsoleSectorsInfo();

		calculateNormalizedVector();
		if (m_needToSave) {
			m_needToSave = false;
			saveNormalizedVectorToFile();
		}

		if (m_needToClassify) {
			m_needToClassify = false;
			classifyImage();
		}
	}

	if (m_classifyMethod == ClassifyMethod::angle) {
		m_anglesCount[0] = 0; 
		m_anglesCount[1] = 0;
		m_anglesCount[2] = 0;
		m_anglesCount[3] = 0;
		processRectangularSectors();
		handleRecursiveAngleMethod({ -1, -1 }, m_maxIter);

		// Sum
		f32 norm = m_anglesCount[0] + m_anglesCount[1] + m_anglesCount[2] + m_anglesCount[3];
		if (m_vectorNormalizationType == VectorNormalization::mod) norm = std::max(std::max(m_anglesCount[0], m_anglesCount[1]), std::max(m_anglesCount[2], m_anglesCount[3]));

		std::cout << "\nVertical   lines | : " << m_anglesCount[0] << "   | " << (f32)m_anglesCount[0] / norm << "\n";
		std::cout << "Diagonal   lines / : " << m_anglesCount[1] << "   | " << (f32)m_anglesCount[1] / norm << "\n";
		std::cout << "Horizontal lines - : " << m_anglesCount[2] << "   | " << (f32)m_anglesCount[2] / norm << "\n";
		std::cout << "Diagonal   lines \\ : " << m_anglesCount[3] << "   | " << (f32)m_anglesCount[3] / norm << "\n";


		if (m_needToSave) {
			m_needToSave = false;
			saveNormalizedVectorToFile();
		}

		if (m_needToClassify) {
			m_needToClassify = false;
			classifyImage();
		}
	}

	if (m_classifyMethod == ClassifyMethod::perceptron || m_classifyMethod == ClassifyMethod::genetic) {
		handlePerceptronProcessing();
		if (m_needToSave) {
			m_needToSave = false;
			saveAIImage();
		}
		handleMaskProcessing();

		if (m_needToSaveWeights) {
			m_needToSaveWeights = false;
			saveWeights();
		}

		if (m_needToLoadWeights) {
			m_needToLoadWeights = false;
			loadWeights();
		}

		if (m_needToClassify) {
			m_needToClassify = false;
			classifyImage();
		}
	}

	updatePreview();
	m_needToUpdate = false;
}

void Classify::endControlWindow() {
	ImGui::End();
}

void Classify::handleImageOpen() {
	float availableWidth = ImGui::GetContentRegionAvail().x;
	ImGui::PushItemWidth(availableWidth);

	std::string buttonText = "OPEN IMAGE";
	ui32 buttonHeight = 40;

	// Label
	std::string label = "Image file path";
	std::string labelID = "##" + label;

	// Input field
	ImGui::InputText("##InputImage", (char*)m_imagePath.c_str(), imagePathBufferSize);
	
	// Hint
	if (!m_imagePath[0]) {
		imguiPadding(12, -fontSize -6);
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7, 0.7, 0.7, 1.0));
		ImGui::Text(label.c_str());
		ImGui::PopStyleColor();
		imguiPadding(0, 2);
	}

	// Button
	if (ImGui::Button(buttonText.c_str(), ImVec2(availableWidth, buttonHeight))) {
		m_needToUpdate = loadImage(m_imagePath);
	}
}

void Classify::handleImageAutoCrop() {
	float availableWidth = ImGui::GetContentRegionAvail().x;
	ImGui::PushItemWidth(availableWidth);

	std::string buttonText = "AUTOCROP IMAGE";
	ui32 buttonHeight = 30;

	// Button
	if (ImGui::Button(buttonText.c_str(), ImVec2(availableWidth, buttonHeight))) {
		m_needToAutoCrop = true;
		m_needToUpdate = true;
	}

	imguiPadding(2, paddingMedium);

	if (ImGui::CollapsingHeader("AutoCrop settings")) {
		std::string rectangularAutoCropText = "Rectangular";
		std::string autoCropPaddingText = "Padding";
		std::string cropFillColorText = "Fill color";
		std::string autoSelectColorText = "COLOR PICKER";
		i32 buttonWidth = availableWidth;
		i32 buttonHeight = 30;

		ui32 textWidth = ImGui::CalcTextSize(rectangularAutoCropText.c_str()).x;
		ImGui::PushItemWidth(availableWidth - textWidth);

		// Padding & fillColor
		ImGui::SliderFloat(autoCropPaddingText.c_str(), &m_autoCropPadding, 0, 1);
		ImGui::SliderFloat3(cropFillColorText.c_str(), &m_autoCropFillColor.r, 0, 1);
		if (ImGui::Button(autoSelectColorText.c_str(), ImVec2(buttonWidth, buttonHeight))) { m_needToSelectAutoCropColor = true; timer.setTimer(500); }

		imguiPadding(0, paddingMedium);

		// Options
		ImGui::Checkbox(rectangularAutoCropText.c_str(), &m_autoCropRectangular);
	}

}

void Classify::handleClassifyMethodSelection() {
	const char* methodNames[] = { "MinMax (checks if subject is in range)",
								  "Average distance (selects minimum error (distance))",
								  "Angle",
								  "Perceptron",
								  "Genetic"};

	std::string label = "Classify method";
	std::string labelID = "##" + label;
	i32 labelHeight = ImGui::CalcTextSize(label.c_str()).y;

	float availableWidth = ImGui::GetContentRegionAvail().x;
	ImGui::PushItemWidth(availableWidth);

	// Window context
	imguiPadding(paddingSmall, 0);
	ImGui::Text(label.c_str()); // Label
	// Combobox. If selected - show elements
	if (ImGui::BeginCombo(labelID.c_str(), methodNames[(ui32)m_classifyMethod])) { 
		// Display each element and allow to select one
		for (ui32 type = 0; type < 5; type++) {
			// Adding prefix
			std::string prefix = " " + std::to_string(type + 1) + ". ";
			ImGui::Text(prefix.c_str());
			ImGui::SameLine();

			// Displaying method type
			if (ImGui::Selectable(methodNames[type])) {
				m_classifyMethod = (ClassifyMethod)type;
				m_needToUpdate = true;
			}
		}
		ImGui::EndCombo();
	}
}

void Classify::handleImageSectorProcessing() {
	float availableWidth = ImGui::GetContentRegionAvail().x;
	ImGui::PushItemWidth(availableWidth);

	// Mask 
	bool equal = false;
	bool eqOrLower = false;
	bool eqOrHigher = false;

	if (m_maskType == MaskType::equal) equal = true;
	if (m_maskType == MaskType::eqOrLower) eqOrLower = true;
	if (m_maskType == MaskType::eqOrHigher) eqOrHigher = true;

	if (ImGui::Checkbox("Count pixels ==  threshold", &equal)) { m_maskType = MaskType::equal; m_needToUpdate = true; }
	if (ImGui::Checkbox("Count pixels <= than threshold", &eqOrLower)) { m_maskType = MaskType::eqOrLower; m_needToUpdate = true; }
	if (ImGui::Checkbox("Count pixels >= than threshold", &eqOrHigher)) { m_maskType = MaskType::eqOrHigher; m_needToUpdate = true; }

	if (ImGui::Checkbox("Display threshold", &m_displayMaskThreshold)) { m_needToUpdate = true; }
	if (ImGui::SliderInt("##handleImageSectorProcessingThreshold", &m_maskThreshold, 0, 255)) { m_needToUpdate = true; }

	imguiPadding(0, paddingMedium);



	// Normalization method selection
	const char* normalizationTypeNames[] = { "Current/Total",
									  "Current/Max" };
	std::string labelNormalization = "Normalization method";
	std::string labelNormalizationID = "##" + labelNormalization;

	// Window context
	imguiPadding(paddingSmall, 0);
	ImGui::Text(labelNormalization.c_str()); // Label
	// Combobox. If selected - show elements
	if (ImGui::BeginCombo(labelNormalizationID.c_str(), normalizationTypeNames[(ui32)m_vectorNormalizationType])) {
		// Display each element and allow to select one
		for (ui32 type = 0; type < 2; type++) {
			// Adding prefix
			std::string prefix = " " + std::to_string(type + 1) + ". ";
			ImGui::Text(prefix.c_str());
			ImGui::SameLine();

			// Displaying method type
			if (ImGui::Selectable(normalizationTypeNames[type])) {
				m_vectorNormalizationType = (VectorNormalization)type;
				m_needToUpdate = true;
			}
		}
		ImGui::EndCombo();
	}

	imguiPadding(0, paddingMedium);



	// Sectors
	const char* sectorTypeNames[] = { "Rectangles",
									  "Circles" };
	std::string label = "Sector type";
	std::string labelID = "##" + label;
	
	// Window context
	imguiPadding(paddingSmall, 0);
	ImGui::Text(label.c_str()); // Label
	// Combobox. If selected - show elements
	if (ImGui::BeginCombo(labelID.c_str(), sectorTypeNames[(ui32)m_sectorsType])) {
		// Display each element and allow to select one
		for (ui32 type = 0; type < 2; type++) {
			// Adding prefix
			std::string prefix = " " + std::to_string(type + 1) + ". ";
			ImGui::Text(prefix.c_str());
			ImGui::SameLine();

			// Displaying method type
			if (ImGui::Selectable(sectorTypeNames[type])) {
				m_sectorsType = (SectorType)type;
				m_needToUpdate = true;
			}
		}
		ImGui::EndCombo();
	}

	if (ImGui::Checkbox("Display sectors", &m_displaySectors)) { m_needToUpdate = true; }

	if (m_sectorsType == SectorType::rectangular) {
		if (ImGui::SliderInt("##handleImageSectorProcessingSectorsHorizontal", &m_horizontalSectorsCnt, 1, 15)) { m_needToUpdate = true; }
		if (ImGui::SliderInt("##handleImageSectorProcessingSectorsVertical", &m_verticalSectorsCnt, 1, 15)) { m_needToUpdate = true; }
	} else if (m_sectorsType == SectorType::circle) {
		if (ImGui::SliderInt("##handleImageSectorProcessingSectorsCircle", &m_circleSectorsCnt, 1, 15)) { m_needToUpdate = true; }
	}


	imguiPadding(0, paddingMedium);

	std::string totalCountedPixelsString = "Counted pixels: " + std::to_string(m_totalSectorsCountedPixels);
	ImGui::Text(totalCountedPixelsString.c_str());

	imguiPadding(0, paddingMedium);


	f32 checkboxHeight = ImGui::GetFrameHeight();
	f32 checkboxWidth = checkboxHeight + ImGui::GetStyle().ItemInnerSpacing.x;

	ImGui::Columns(m_classesCount, 0, false);

	for (i32 i = 0; i < m_classesCount; i++) {
		std::string id = "##handleImageSectorProcessingCheckbox_" + std::to_string(i);
		ImGui::Checkbox(id.c_str(), &m_classesCheckboxes[i]);
		ImGui::NextColumn(); 
	}
	ImGui::Columns(1);

	imguiPadding(0, -checkboxHeight);
	ImGui::Columns(m_classesCount, 0, false);

	for (i32 i = 0; i < m_classesCount; i++) {
		std::string id = "  " + std::to_string(i);
		f32v4 circleColor = m_classesCheckboxes[i] ? f32v4(0.4, 0.4, 0.8, 1) : f32v4(0, 0, 0, 0);
		f32v4 textColor = { 1, 1, 1, 1 };
		i32v2 circleCenter = { ImGui::GetCursorScreenPos().x + checkboxHeight / 2, ImGui::GetCursorScreenPos().y + checkboxHeight / 2 - 3 };

		drawCircle(circleCenter, checkboxHeight / 2, circleColor, false, true);

		setTextColor(textColor);
		ImGui::Text(id.c_str());
		endTextColor();

		ImGui::NextColumn();
	}
	ImGui::Columns(1);

	imguiPadding(0, paddingMedium);

	// Save/Classify/Clear
	std::string saveButtonText = "SAVE";
	std::string classifyButtonText = "CLASSIFY";
	std::string clearFolderButtonText = "CLEAR FOLDER";

	i32 buttonPadding = 4;
	if (ImGui::Button(saveButtonText.c_str(), ImVec2(availableWidth / 2 - buttonPadding, buttonHeightMedium))) { m_needToUpdate = true;	m_needToSave = true; }
	ImGui::SameLine();
	if (ImGui::Button(classifyButtonText.c_str(), ImVec2(availableWidth / 2, buttonHeightMedium))) { m_needToUpdate = true; m_needToClassify = true; }
	if( ImGui::Button(clearFolderButtonText.c_str(), ImVec2(availableWidth, buttonHeightSmall))) { 
		ent::io::clearDirectory(ent::algorithm::stringToWstring(m_classifyFolderPath));
	}
}

void Classify::handleImageProcessing() {
	float availableWidth = ImGui::GetContentRegionAvail().x;
	ImGui::PushItemWidth(availableWidth);

	if (ImGui::CollapsingHeader("Image settings")) {
		std::string brightnessText = "Brightness";
		std::string contrasText = "Contrast";
		ui32 textWidth = ImGui::CalcTextSize(brightnessText.c_str()).x;
		ImGui::PushItemWidth(availableWidth - textWidth);

		// Brightness
		if (ImGui::SliderFloat(brightnessText.c_str(), &m_imageBrightness, -128, 127)) { m_needToUpdate = true; }

		// Contrast
		if (ImGui::SliderFloat(contrasText.c_str(), &m_imageContrast, -128, 127)) { m_needToUpdate = true; }

		imguiPadding(0, paddingMedium);

		// Options
		if (ImGui::Checkbox("Black and White", &m_imageBlackAndWhite)) { m_needToUpdate = true; }
		if (ImGui::Checkbox("Invert colors", &m_imageInvertColors))  { m_needToUpdate = true; }
	}
}

void Classify::handleMinMaxMethod() {
	handleImageSectorProcessing();
}

void Classify::handleAverageDistMethod() {
	handleImageSectorProcessing();
}

void Classify::handlePerceptronMethod() {
	float availableWidth = ImGui::GetContentRegionAvail().x;
	ImGui::PushItemWidth(availableWidth);

	// Mask 
	bool equal = false;
	bool eqOrLower = false;
	bool eqOrHigher = false;

	if (m_maskType == MaskType::equal) equal = true;
	if (m_maskType == MaskType::eqOrLower) eqOrLower = true;
	if (m_maskType == MaskType::eqOrHigher) eqOrHigher = true;

	if (ImGui::Checkbox("Count pixels ==  threshold", &equal)) { m_maskType = MaskType::equal; m_needToUpdate = true; }
	if (ImGui::Checkbox("Count pixels <= than threshold", &eqOrLower)) { m_maskType = MaskType::eqOrLower; m_needToUpdate = true; }
	if (ImGui::Checkbox("Count pixels >= than threshold", &eqOrHigher)) { m_maskType = MaskType::eqOrHigher; m_needToUpdate = true; }

	if (ImGui::Checkbox("Display threshold", &m_displayMaskThreshold)) { m_needToUpdate = true; }
	if (ImGui::SliderInt("##handleImageSectorProcessingThreshold", &m_maskThreshold, 0, 255)) { m_needToUpdate = true; }

	imguiPadding(0, paddingMedium);


	std::string errorText = std::string("AI Error: ") + std::to_string(m_aiError);
	ImGui::Text(errorText.c_str());

	f32 checkboxHeight = ImGui::GetFrameHeight();
	f32 checkboxWidth = checkboxHeight + ImGui::GetStyle().ItemInnerSpacing.x;

	ImGui::Columns(m_classesCount, 0, false);

	for (i32 i = 0; i < m_classesCount; i++) {
		std::string id = "##handleImageSectorProcessingCheckbox_" + std::to_string(i);
		ImGui::Checkbox(id.c_str(), &m_classesCheckboxes[i]);
		ImGui::NextColumn();
	}
	ImGui::Columns(1);

	imguiPadding(0, -checkboxHeight);
	ImGui::Columns(m_classesCount, 0, false);

	for (i32 i = 0; i < m_classesCount; i++) {
		std::string id = "  " + std::to_string(i);
		f32v4 circleColor = m_classesCheckboxes[i] ? f32v4(0.4, 0.4, 0.8, 1) : f32v4(0, 0, 0, 0);
		f32v4 textColor = { 1, 1, 1, 1 };
		i32v2 circleCenter = { ImGui::GetCursorScreenPos().x + checkboxHeight / 2, ImGui::GetCursorScreenPos().y + checkboxHeight / 2 - 3 };

		drawCircle(circleCenter, checkboxHeight / 2, circleColor, false, true);

		setTextColor(textColor);
		ImGui::Text(id.c_str());
		endTextColor();

		ImGui::NextColumn();
	}
	ImGui::Columns(1);

	imguiPadding(0, paddingMedium);

	// Save/Classify/Clear
	std::string saveButtonText = "SAVE";
	std::string classifyButtonText = "CLASSIFY";
	std::string trainButtonText = "TRAING";
	std::string stopButtonText = "STOP";
	std::string saveWeightsButtonText = "SAVE WEIGHTS";
	std::string loadWeightsButtonText = "LOAD WEIGHTS";
	std::string clearFolderButtonText = "CLEAR FOLDER";

	i32 buttonPadding = 4;
	if (ImGui::Button(saveButtonText.c_str(), ImVec2(availableWidth / 2 - buttonPadding, buttonHeightMedium))) { m_needToUpdate = true;	m_needToSave = true; }
	ImGui::SameLine();
	if (ImGui::Button(classifyButtonText.c_str(), ImVec2(availableWidth / 2, buttonHeightMedium))) { m_needToUpdate = true; m_needToClassify = true; }
	
	if (ImGui::Button(trainButtonText.c_str(), ImVec2(availableWidth / 2 - buttonPadding, buttonHeightMedium))) { m_needToUpdate = true; m_needToTrainAI = true; }
	ImGui::SameLine(); 
	if (ImGui::Button(stopButtonText.c_str(), ImVec2(availableWidth / 2, buttonHeightMedium))) { m_needToUpdate = true; m_needToTrainAI = false; }

	if (ImGui::Button(saveWeightsButtonText.c_str(), ImVec2(availableWidth / 2 - buttonPadding, buttonHeightMedium))) { m_needToUpdate = true; m_needToSaveWeights = true; }
	ImGui::SameLine();
	if (ImGui::Button(loadWeightsButtonText.c_str(), ImVec2(availableWidth / 2, buttonHeightMedium))) { m_needToUpdate = true; m_needToLoadWeights = true; }

	if (ImGui::Button(clearFolderButtonText.c_str(), ImVec2(availableWidth, buttonHeightMedium))) {
		ent::io::clearDirectory(ent::algorithm::stringToWstring(m_classifyFolderPath));
	}


	if(m_imageAITextureID) ImGui::Image((void*)m_imageAITextureID, ImVec2(availableWidth, availableWidth));
	
	std::string widthText =        "Width";
	std::string heightText =       "Height";
	std::string layersCountText =  "Layers count";
	std::string batchSizeText =    "Batch size";
	std::string learningRateText = "Learning rate";

	ui32 textWidth = ImGui::CalcTextSize(learningRateText.c_str()).x;
	ImGui::PushItemWidth(availableWidth - textWidth);

	if (ImGui::SliderInt(widthText.c_str(), &m_imageAISize.x, 1, 256)) { m_needToUpdate = true; m_needToUpdateAI = true; }
	if (ImGui::SliderInt(heightText.c_str(), &m_imageAISize.y, 1, 256)) { m_needToUpdate = true; m_needToUpdateAI = true; }
	if (ImGui::SliderInt(layersCountText.c_str(), &m_layersCount, 2, 10)) { m_needToUpdate = true; m_needToUpdateAI = true; }
	if (ImGui::SliderInt(batchSizeText.c_str(), &m_batchSize, 1, 100)) { m_needToUpdate = true; }
	if (ImGui::SliderFloat (learningRateText.c_str(), &m_learningRate, 0, 0.2)) { m_needToUpdate = true; }

}

void Classify::handleGeneticMethod() {
	float availableWidth = ImGui::GetContentRegionAvail().x;
	ImGui::PushItemWidth(availableWidth);

	// Mask 
	bool equal = false;
	bool eqOrLower = false;
	bool eqOrHigher = false;

	if (m_maskType == MaskType::equal) equal = true;
	if (m_maskType == MaskType::eqOrLower) eqOrLower = true;
	if (m_maskType == MaskType::eqOrHigher) eqOrHigher = true;

	if (ImGui::Checkbox("Count pixels ==  threshold", &equal)) { m_maskType = MaskType::equal; m_needToUpdate = true; }
	if (ImGui::Checkbox("Count pixels <= than threshold", &eqOrLower)) { m_maskType = MaskType::eqOrLower; m_needToUpdate = true; }
	if (ImGui::Checkbox("Count pixels >= than threshold", &eqOrHigher)) { m_maskType = MaskType::eqOrHigher; m_needToUpdate = true; }

	if (ImGui::Checkbox("Display threshold", &m_displayMaskThreshold)) { m_needToUpdate = true; }
	if (ImGui::SliderInt("##handleImageSectorProcessingThreshold", &m_maskThreshold, 0, 255)) { m_needToUpdate = true; }

	imguiPadding(0, paddingMedium);


	f32 checkboxHeight = ImGui::GetFrameHeight();
	f32 checkboxWidth = checkboxHeight + ImGui::GetStyle().ItemInnerSpacing.x;

	ImGui::Columns(m_classesCount, 0, false);

	for (i32 i = 0; i < m_classesCount; i++) {
		std::string id = "##handleImageSectorProcessingCheckbox_" + std::to_string(i);
		ImGui::Checkbox(id.c_str(), &m_classesCheckboxes[i]);
		ImGui::NextColumn();
	}
	ImGui::Columns(1);

	imguiPadding(0, -checkboxHeight);
	ImGui::Columns(m_classesCount, 0, false);

	for (i32 i = 0; i < m_classesCount; i++) {
		std::string id = "  " + std::to_string(i);
		f32v4 circleColor = m_classesCheckboxes[i] ? f32v4(0.4, 0.4, 0.8, 1) : f32v4(0, 0, 0, 0);
		f32v4 textColor = { 1, 1, 1, 1 };
		i32v2 circleCenter = { ImGui::GetCursorScreenPos().x + checkboxHeight / 2, ImGui::GetCursorScreenPos().y + checkboxHeight / 2 - 3 };

		drawCircle(circleCenter, checkboxHeight / 2, circleColor, false, true);

		setTextColor(textColor);
		ImGui::Text(id.c_str());
		endTextColor();

		ImGui::NextColumn();
	}
	ImGui::Columns(1);

	imguiPadding(0, paddingMedium);

	// Save/Classify/Clear
	std::string saveButtonText = "SAVE";
	std::string classifyButtonText = "CLASSIFY";
	std::string clearFolderButtonText = "CLEAR FOLDER";

	i32 buttonPadding = 4;
	if (ImGui::Button(saveButtonText.c_str(), ImVec2(availableWidth / 2 - buttonPadding, buttonHeightMedium))) { m_needToUpdate = true;	m_needToSave = true; }
	ImGui::SameLine();
	if (ImGui::Button(classifyButtonText.c_str(), ImVec2(availableWidth / 2, buttonHeightMedium))) { m_needToUpdate = true; m_needToClassify = true; }
	if (ImGui::Button(clearFolderButtonText.c_str(), ImVec2(availableWidth, buttonHeightSmall))) {
		ent::io::clearDirectory(ent::algorithm::stringToWstring(m_classifyFolderPath));
	}
}

void Classify::imguiPadding(i32 x, i32 y) {
	if (x) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + x);
	if (y) ImGui::SetCursorPosY(ImGui::GetCursorPosY() + y);
}

void Classify::drawCircle(i32v2& center, f32 radius, f32v4 color, bool alwaysOnTop, bool filled, float thickness, int segments) {
	ImDrawList* draw_list = alwaysOnTop ? ImGui::GetForegroundDrawList() : ImGui::GetWindowDrawList();

	if (filled) {
		draw_list->AddCircleFilled(ImVec2(center.x, center.y), radius, ImColor(color.r, color.g, color.b, color.a), segments);
	} else {
		draw_list->AddCircle(ImVec2(center.x, center.y), radius, ImColor(color.r, color.g, color.b, color.a), segments, thickness);
	}
}

ui32 Classify::createTexture(ui8* data, i32 width, i32 height, i32 nChannels) {
	if (!data) {
		return 0;
	}

	ui32 textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	ui32 mode;
	if (nChannels == 1) {
		mode = GL_RED;
	} else if (nChannels == 3) {
		mode = GL_RGB;
	} else if (nChannels == 4) {
		mode = GL_RGBA;
	} else {
		glDeleteTextures(1, &textureID);
		return 0;
	}

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glTexImage2D(GL_TEXTURE_2D, 0, mode, width, height, 0, mode, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	return textureID;
}

void Classify::handleImageSelection() {
	if (m_needToSelectAutoCropColor) return; // Return if picking a color

	i32v2 imageOnScreenStart;
	i32v2 imageOnScreenSize;
	fitImageIntoRectangle(m_imageSize, { m_window.getWidth() / 2, m_window.getHeight() / 2 }, &imageOnScreenStart, &imageOnScreenSize);
	i32v2 imageOnScreenEnd = imageOnScreenStart + imageOnScreenSize;

	imageOnScreenStart.x -= 2;
	imageOnScreenStart.y -= 0;
	imageOnScreenEnd.x -= 2;
	imageOnScreenEnd.y -= 1;

	drawSelection(imageOnScreenStart, imageOnScreenEnd);
}

void Classify::deleteTexture(ui32 textureID) {
	glDeleteTextures(1, &textureID);
}

void Classify::drawRectangle(i32v2 position, i32v2 size, f32v4 color, bool filled , float outlineThickness, bool alwaysOnTop) {
	// Get the current window's draw list
	ImDrawList* draw_list = alwaysOnTop ? ImGui::GetForegroundDrawList() : ImGui::GetWindowDrawList();

	ImVec2 rect_min = ImVec2(position.x, position.y);					  // Top-left corner
	ImVec2 rect_max = ImVec2(position.x + size.x, position.y + size.y);	  // Bottom-right corner

	ImColor col = ImColor((i32)(color.r * 255.0), (i32)(color.g * 255.0), (i32)(color.b * 255.0), (i32)(color.a * 255.0));

	// Draw the filled rectangle
	if (filled) draw_list->AddRectFilled(rect_min, rect_max, col);

	// Draw the outline
	draw_list->AddRect(rect_min, rect_max, col, 0.0f, 0, outlineThickness);
}

void Classify::drawPattern(std::string pattern, f32v4 color) {
	float availableWidth = ImGui::GetContentRegionAvail().x;
	ImGuiStyle style = ImGui::GetStyle();

	std::string line;
	float currentLineWidth = 0.0f;
	for (size_t i = 0; currentLineWidth < availableWidth; i++) {
		char nextChar = pattern[i % pattern.size()];  // Get the next character from the pattern
		std::string nextCharStr(1, nextChar);  // Convert char to string

		// Measure the width of the next character
		ImVec2 charSize = ImGui::CalcTextSize(nextCharStr.c_str());
		if (currentLineWidth + charSize.x > availableWidth + style.WindowPadding.x) {
			break;  // Stop if the next character exceeds available width
		}

		line += nextChar;
		currentLineWidth += charSize.x;
	}

	setTextColor(color);
	ImGui::Text(line.c_str());
	endTextColor();
}

void Classify::setTextColor(f32v4 color) {
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(color.r, color.g, color.b, color.a));
}

void Classify::endTextColor() {
	ImGui::PopStyleColor();
}

void Classify::fitImageIntoRectangle(i32v2 imageSize, i32v2 rectSize, i32v2* outPosition, i32v2* outSize) {
	float aspectRatio = (float)imageSize.x / (float)imageSize.y;
	float scaleFactor = std::min((float)rectSize.x / imageSize.x, (float)rectSize.y / imageSize.y);
	float scaledWidth = imageSize.x * scaleFactor;
	float scaledHeight = imageSize.y * scaleFactor;

	// Image position at the center + maximizing size ensuring aspect ratio
	*outPosition = ui32v2((rectSize.x - scaledWidth) / 2, (rectSize.y - scaledHeight) / 2);
	*outSize = ui32v2(scaledWidth, scaledHeight);
}

void Classify::drawImage(i32v2 windowPosition, i32v2 windowSize, i32 imageTextureId, i32v2 imageSize) {
	// Window id
	std::string windowName = "##drawImage_" + std::to_string(windowPosition.x) + "_" +
		std::to_string(windowPosition.y) + "_" +
		std::to_string(windowSize.x) + "_" +
		std::to_string(windowSize.x) + "_" +
		std::to_string(imageTextureId) + "_" +
		std::to_string(imageTextureId) + "_" +
		std::to_string(imageTextureId);

	// Creating window
	ImGui::SetNextWindowPos(ImVec2(windowPosition.x, windowPosition.y));
	ImGui::SetNextWindowSize(ImVec2(windowSize.x, windowSize.y));
	ImGui::Begin(windowName.c_str(), nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

	// Rendering Image + data
	if (imageTextureId) {
		i32v2 imgPos;
		i32v2 imgSize;

		fitImageIntoRectangle(imageSize, windowSize, &imgPos, &imgSize);

		// Rendering image
		ImGui::SetCursorPos(ImVec2(imgPos.x, imgPos.y));
		ImGui::Image((void*)imageTextureId, ImVec2(imgSize.x, imgSize.y));


		// Displaying image size in pixels 
		std::string imgSizeText = std::to_string(imageSize.x) + "x" + std::to_string(imageSize.y);
		ImVec2 textSize = ImGui::CalcTextSize(imgSizeText.c_str());
		ui32v2 renderTextPosition = { windowSize.x - textSize.x - 5, 5 };
		ui32v2 renderRectanglePosition = {windowSize.x - textSize.x - 10, windowPosition.y };
		ui32v2 rectanglePos = {renderTextPosition.x - 5, renderTextPosition.y - 5 };
		ui32v2 rectangleSize = { textSize.x + 10, textSize.y + 10 };

		// Rendering background rectangle
		drawRectangle(renderRectanglePosition, rectangleSize, {0, 0, 0, 0.75}, true);

		// Rendering text
		ImGui::SetCursorPos(ImVec2(renderTextPosition.x, renderTextPosition.y));
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		ImGui::Text("%s", imgSizeText.c_str());
		ImGui::PopStyleColor();

	} else { // Rendering "No image" text
		const char* text = "No image";
		ImVec2 textSize = ImGui::CalcTextSize(text);
		ImVec2 textPos = ImVec2((windowSize.x - textSize.x) / 2, (windowSize.y - textSize.y) / 2);
		ImGui::SetCursorPos(textPos);
		ImGui::Text("%s", text);
	}

	ImGui::End();
}

void Classify::handleRecursiveAngleMethod(i32v2 centerPoint, int n) {
	if (n < 1) return;
	
	std::vector<i32v2> newPoints = handleAngleMethodProcessing(centerPoint);

	for (i32 i = 0; i < newPoints.size(); i++) {
		handleRecursiveAngleMethod(newPoints[i], n - 1);
	}
}

std::vector<i32v2> Classify::handleAngleMethodProcessing(i32v2 centerPoint) {
	std::vector<i32v2> centerPoints;

	ui32v2 first_point = centerPoint;

	// Find first point in case not set
	if (centerPoint.x < 0 || centerPoint.y < 0) {
		bool found_first_point = false;
		for (int y = 0; y < m_imageSize.y && !found_first_point; y++) {
			for (int x = 0; x < m_imageSize.x && !found_first_point; x++) {
				ui32 index = y * m_imageSize.x * m_imageChannelsCnt + x * m_imageChannelsCnt;
				if (m_imageMask[index]) {
					first_point = { x, y };
					found_first_point = true;
				}
			}
		}
		if (!found_first_point) return centerPoints;
	}

	// Finding points
	i32v2 top_left = first_point; top_left -= m_circleOffset * m_circleRadius;
	i32v2 bottom_right = first_point; bottom_right += m_circleOffset * m_circleRadius;

	if (top_left.x < 0) top_left.x = 0;
	if (top_left.y < 0) top_left.y = 0;

	if (bottom_right.x > m_imageSize.x - 1) bottom_right.x = m_imageSize.x - 1;
	if (bottom_right.y > m_imageSize.y - 1) bottom_right.y = m_imageSize.y - 1;


	i32 min_size = 5;
	if (bottom_right.x - top_left.x < min_size) return centerPoints;
	if (bottom_right.y - top_left.y < min_size) return centerPoints;


	// Finding mid point
	if (centerPoint.x < 0 || centerPoint.y < 0) {
		std::vector<ui32> x_coords;
		std::vector<ui32> y_coords;

		for (i32 y = top_left.y; y < bottom_right.y; y++) {
			for (i32 x = top_left.x; x < bottom_right.x; x++) {
				ui32 index = y * m_imageSize.x * m_imageChannelsCnt + x * m_imageChannelsCnt;
				if (m_imageMask[index]) {
					x_coords.push_back(x);
					y_coords.push_back(y);
				}
			}
		}

		// Mean
		i32 x_sum = 0;
		for (int i = 0; i < x_coords.size(); i++) {
			x_sum += x_coords[i];
		}

		i32 y_sum = 0;
		for (int i = 0; i < y_coords.size(); i++) {
			y_sum += y_coords[i];
		}

		if (!x_coords.size()) return centerPoints;
		if (!y_coords.size()) return centerPoints;

		i32 x_mean = x_sum / x_coords.size();
		i32 y_mean = y_sum / y_coords.size();

		// New point
		first_point = { x_mean, y_mean };
		top_left = first_point; top_left -= m_circleOffset * m_circleRadius;
		bottom_right = first_point; bottom_right += m_circleOffset * m_circleRadius;

		if (top_left.x < 0) top_left.x = 0;
		if (top_left.y < 0) top_left.y = 0;

		if (bottom_right.x > m_imageSize.x) bottom_right.x = m_imageSize.x - 1;
		if (bottom_right.y > m_imageSize.y) bottom_right.y = m_imageSize.y - 1;

		if (bottom_right.x - top_left.x < min_size) return centerPoints;
		if (bottom_right.y - top_left.y < min_size) return centerPoints;
	}

	
	// Circle
	// Converting circle to line
	f32 pi = 3.14159265368979;
	i32 ind = 0;

	std::vector<bool> line;
	line.resize(m_circlePrecision, 0);

	for (f32 a = 0; a < m_circlePrecision; a ++) {
		f32 a_norm = a / m_circlePrecision;
		f32 x = std::sin(a_norm * pi * 2);
		f32 y = std::cos(a_norm * pi * 2);

		f32 r = m_circleRadius * m_circleOffset - 1;
		x = (int)(first_point.x + x * r);
		y = (int)(first_point.y + y * r);

		ind++;

		if (x < 0) continue;
		if (y < 0) continue;

		if (x > m_imageSize.x - 1) continue;
		if (y > m_imageSize.y - 1) continue;

		ui32 index = y * m_imageSize.x * m_imageChannelsCnt + x * m_imageChannelsCnt;

		if (m_displayAngleAux) {
			m_imagePreview[index + 0] = 255;
			m_imagePreview[index + 1] = 0;
			m_imagePreview[index + 2] = 0;
		}

		
		line[ind - 1] = 0;
		if (m_imageMask[index] && !m_imageMask[index + 1]) {
			line[ind - 1] = 1;

			if (m_displayAngleAux) {
				m_imagePreview[index + 0] = 0;
				m_imagePreview[index + 1] = 255;
				m_imagePreview[index + 2] = 0;
			}
		}

	}

	// Drawing rectangle
	if (m_displayRectangles) {
		for (int x = top_left.x; x < bottom_right.x; x++) {
			ui32 index1 = top_left.y * m_imageSize.x * m_imageChannelsCnt + x * m_imageChannelsCnt;
			ui32 index2 = bottom_right.y * m_imageSize.x * m_imageChannelsCnt + x * m_imageChannelsCnt;

			m_imagePreview[index1] = 255;
			m_imagePreview[index2] = 255;

			m_imagePreview[index1 + 1] = 0;
			m_imagePreview[index2 + 1] = 0;

			m_imagePreview[index1 + 2] = 0;
			m_imagePreview[index2 + 2] = 0;
		}


		for (int y = top_left.y; y < bottom_right.y; y++) {
			ui32 index1 = y * m_imageSize.x * m_imageChannelsCnt + top_left.x * m_imageChannelsCnt;
			ui32 index2 = y * m_imageSize.x * m_imageChannelsCnt + bottom_right.x * m_imageChannelsCnt;

			m_imagePreview[index1] = 255;
			m_imagePreview[index2] = 255;

			m_imagePreview[index1 + 1] = 0;
			m_imagePreview[index2 + 1] = 0;

			m_imagePreview[index1 + 2] = 0;
			m_imagePreview[index2 + 2] = 0;
		}
	}

	// Filling mask marking where we already were
	for (i32 y = top_left.y; y < bottom_right.y; y++) {
		for (i32 x = top_left.x; x < bottom_right.x; x++) {
			ui32 index = y * m_imageSize.x * m_imageChannelsCnt + x * m_imageChannelsCnt;
			i32 y_norm = first_point.y - y;
			i32 x_norm = first_point.x - x;

			// Circle formula a^2 + b^2 = r^2
			f32 a = y_norm * y_norm + x_norm * x_norm;
			f32 r = m_circleRadius * m_circleOffset - 1;
			if (a < r * r) {
				m_imageMask[index + 1] = 1;
			}
		}
	}



	// Check that image has direction
	bool bad_image = true;
	for (i32 i = 0; i < line.size(); i++) {
		if (line[i] != 0) bad_image = false;
	}
	if (bad_image) return centerPoints;

	// Normalize circle
	// If line has sector that is separated
	bool need_to_normalize = line[0] && line[line.size() - 1];
	std::vector<bool> line_normalized;
	line_normalized.resize(line.size(), 0);
	i32 line_offset = 0;

	// Copy
	for (i32 i = 0; i < line.size(); i++) {
		line_normalized[i] = line[i];
	}


	if (need_to_normalize) {
		for (i32 i = line.size() - 1; i > 0; i--) {
			if (!line[i]) break;
			line_offset++;
		}

		for (i32 i = 0; i < line.size(); i++) {
			i32 index = line.size() - line_offset + i;
			if (index >= line.size()) index -= line.size();
			line_normalized[i] = line[index];
		}
	}



	// Extracting sectors midpoints
	i32 sector_start = 0;
	i32 sector_end = 0;

	std::vector<f32> sector_mid_points;

	bool need_to_find_start = true;
	for (i32 i = 0; i < line.size(); i++) {
		if(need_to_find_start && line_normalized[i]) {
			sector_start = i;
			need_to_find_start = false;
		}
		if ((!line_normalized[i] && !need_to_find_start) || (i == line.size() - 1 && line_normalized[i])) {
			sector_end = i - 1;
			f32 mid = sector_start + (f32)(sector_end - sector_start) / 2.0;
			if (sector_start == sector_end) mid = sector_start;

			mid -= line_offset;
			mid = (mid / m_circlePrecision);
			if (mid < 0) mid = 1.0 + mid;
			sector_mid_points.push_back(mid);
			need_to_find_start = true;
		}
	}


	// Combining sectors based on threshold
	std::vector<f32> combined_midpoints = combineMidpoints(sector_mid_points, m_CombineAngleThreshold);


	// Tesing!!! Displaying lines
	for (f32 i = 0; i < combined_midpoints.size(); i++) {
		f32 a = combined_midpoints[i];
		f32 x = std::sin(a * pi * 2);
		f32 y = std::cos(a * pi * 2);

		f32 r = m_circleRadius * m_circleOffset - 1;
		x = (int)(first_point.x + x * r);
		y = (int)(first_point.y + y * r);

		for (f32 t = 0; t < m_lerpIter; t++) {
			i32 y_f = ent::math::lerp(first_point.y, y, t / m_lerpIter);
			i32 x_f = ent::math::lerp(first_point.x, x, t / m_lerpIter);

			if (x_f < 0) continue;
			if (y_f < 0) continue;

			if (x_f > m_imageSize.x - 1) continue;
			if (y_f > m_imageSize.y - 1) continue;

			ui32 index = y_f * m_imageSize.x * m_imageChannelsCnt + x_f * m_imageChannelsCnt;

			m_imagePreview[index + 0] = 0;
			m_imagePreview[index + 1] = 0;
			m_imagePreview[index + 2] = 255;
		}
	}


	// Counting angles count based on their angle
	// x > 0.9375 && x < 0.0625 (bottom)         x < 0.5625 && x > 0.4375 (top)         
	// x > 0.8125 && x < 0.9375 (bottom left)    x < 0.4375 && x > 0.3125 (top right)    
	// x > 0.6875 && x < 0.8125 (left)           x < 0.3125 && x > 0.1875 (left)        
	// x > 0.5625 && x < 0.6875 (top left)       x < 0.1875 && x > 0.0625 (bottom right)

	for (int i = 0; i < combined_midpoints.size(); i++) {
		f32 a = combined_midpoints[i];

		if ((a > 0.9375 || a < 0.0625) || (a < 0.5625 && a > 0.4375)) m_anglesCount[0]++; // Vertical   line |
		if ((a > 0.8125 && a < 0.9375) || (a < 0.4375 && a > 0.3125)) m_anglesCount[1]++; // Diagonal   line /
		if ((a > 0.6875 && a < 0.8125) || (a < 0.3125 && a > 0.1875)) m_anglesCount[2]++; // Horizontal line -
		if ((a > 0.5625 && a < 0.6875) || (a < 0.1875 && a > 0.0625)) m_anglesCount[3]++; // Diagonal   line \;
	}

	// Converting values to coordinates
	for (f32 i = 0; i < combined_midpoints.size(); i++) {
		f32 a = combined_midpoints[i];
		f32 x = std::sin(a * pi * 2);
		f32 y = std::cos(a * pi * 2);

		f32 r = m_circleRadius * m_circleOffset - 1;
		x = (int)(first_point.x + x * r);
		y = (int)(first_point.y + y * r);

		if (x < 0) continue;
		if (y < 0) continue;

		if (x > m_imageSize.x - 1) continue;
		if (y > m_imageSize.y - 1) continue;

		centerPoints.push_back({ x, y });
	}

	return centerPoints;
}


void Classify::handleAngleMethod() {
	float availableWidth = ImGui::GetContentRegionAvail().x;
	ImGui::PushItemWidth(availableWidth);

	if (ImGui::CollapsingHeader("Angle settings")) {
		std::string radiusText  = "Circle radius";
		std::string combineText = "Combine threshold";
		std::string angleText   = "Angles count";
		std::string iterText    = "Max iterations";
		std::string lerpText    = "Lerp iterations";
		ui32 textWidth = ImGui::CalcTextSize(combineText.c_str()).x;
		ImGui::PushItemWidth(availableWidth - textWidth);

		if (ImGui::SliderFloat(radiusText.c_str(), &m_circleRadius, 5, 100)) { m_needToUpdate = true; }
		if (ImGui::SliderFloat(combineText.c_str(), &m_CombineAngleThreshold, 0, 1)) { m_needToUpdate = true; }
		if (ImGui::SliderInt(angleText.c_str(), &m_circlePrecision, 10, 720)) { m_needToUpdate = true; }
		if (ImGui::SliderInt(iterText.c_str(), &m_maxIter, 1, 256)) { m_needToUpdate = true; }
		if (ImGui::SliderInt(lerpText.c_str(), &m_lerpIter, 1, 256)) { m_needToUpdate = true; }

		imguiPadding(0, paddingSmall);

		if (ImGui::Checkbox("Display Angle auxilarity", &m_displayAngleAux)) { m_needToUpdate = true; }
		if (ImGui::Checkbox("Display Rectangles", &m_displayRectangles)) { m_needToUpdate = true; }		
	}

	imguiPadding(0, paddingMedium);

	ImGui::PushItemWidth(availableWidth );


	// Normalization method selection
	const char* normalizationTypeNames[] = { "Current/Total",
									  "Current/Max" };
	std::string labelNormalization = "Normalization method";
	std::string labelNormalizationID = "##" + labelNormalization;

	// Window context
	imguiPadding(paddingSmall, 0);
	ImGui::Text(labelNormalization.c_str()); // Label
	// Combobox. If selected - show elements
	if (ImGui::BeginCombo(labelNormalizationID.c_str(), normalizationTypeNames[(ui32)m_vectorNormalizationType])) {
		// Display each element and allow to select one
		for (ui32 type = 0; type < 2; type++) {
			// Adding prefix
			std::string prefix = " " + std::to_string(type + 1) + ". ";
			ImGui::Text(prefix.c_str());
			ImGui::SameLine();

			// Displaying method type
			if (ImGui::Selectable(normalizationTypeNames[type])) {
				m_vectorNormalizationType = (VectorNormalization)type;
				m_needToUpdate = true;
			}
		}
		ImGui::EndCombo();
	}

	imguiPadding(0, paddingMedium);


	// Mask 
	bool equal = false;
	bool eqOrLower = false;
	bool eqOrHigher = false;

	if (m_maskType == MaskType::equal) equal = true;
	if (m_maskType == MaskType::eqOrLower) eqOrLower = true;
	if (m_maskType == MaskType::eqOrHigher) eqOrHigher = true;

	if (ImGui::Checkbox("Count pixels ==  threshold", &equal)) { m_maskType = MaskType::equal; m_needToUpdate = true; }
	if (ImGui::Checkbox("Count pixels <= than threshold", &eqOrLower)) { m_maskType = MaskType::eqOrLower; m_needToUpdate = true; }
	if (ImGui::Checkbox("Count pixels >= than threshold", &eqOrHigher)) { m_maskType = MaskType::eqOrHigher; m_needToUpdate = true; }

	if (ImGui::Checkbox("Display threshold", &m_displayMaskThreshold)) { m_needToUpdate = true; }
	if (ImGui::SliderInt("##handleImageSectorProcessingThreshold", &m_maskThreshold, 0, 255)) { m_needToUpdate = true; }

	imguiPadding(0, paddingMedium);





	f32 checkboxHeight = ImGui::GetFrameHeight();
	f32 checkboxWidth = checkboxHeight + ImGui::GetStyle().ItemInnerSpacing.x;

	ImGui::Columns(m_classesCount, 0, false);

	for (i32 i = 0; i < m_classesCount; i++) {
		std::string id = "##handleImageSectorProcessingCheckbox_" + std::to_string(i);
		ImGui::Checkbox(id.c_str(), &m_classesCheckboxes[i]);
		ImGui::NextColumn();
	}
	ImGui::Columns(1);

	imguiPadding(0, -checkboxHeight);
	ImGui::Columns(m_classesCount, 0, false);

	for (i32 i = 0; i < m_classesCount; i++) {
		std::string id = "  " + std::to_string(i);
		f32v4 circleColor = m_classesCheckboxes[i] ? f32v4(0.4, 0.4, 0.8, 1) : f32v4(0, 0, 0, 0);
		f32v4 textColor = { 1, 1, 1, 1 };
		i32v2 circleCenter = { ImGui::GetCursorScreenPos().x + checkboxHeight / 2, ImGui::GetCursorScreenPos().y + checkboxHeight / 2 - 3 };

		drawCircle(circleCenter, checkboxHeight / 2, circleColor, false, true);

		setTextColor(textColor);
		ImGui::Text(id.c_str());
		endTextColor();

		ImGui::NextColumn();
	}
	ImGui::Columns(1);

	imguiPadding(0, paddingMedium);

	// Save/Classify/Clear
	std::string saveButtonText = "SAVE";
	std::string classifyButtonText = "CLASSIFY";
	std::string clearFolderButtonText = "CLEAR FOLDER";

	i32 buttonPadding = 4;
	if (ImGui::Button(saveButtonText.c_str(), ImVec2(availableWidth / 2 - buttonPadding, buttonHeightMedium))) { m_needToUpdate = true;	m_needToSave = true; }
	ImGui::SameLine();
	if (ImGui::Button(classifyButtonText.c_str(), ImVec2(availableWidth / 2, buttonHeightMedium))) { m_needToUpdate = true; m_needToClassify = true; }
	if (ImGui::Button(clearFolderButtonText.c_str(), ImVec2(availableWidth, buttonHeightSmall))) {
		ent::io::clearDirectory(ent::algorithm::stringToWstring(m_classifyFolderPath));
	}
}



std::vector<f32> Classify::combineMidpoints(const std::vector<f32>& midpoints, f32 threshold) {
	std::vector<float> combinedMidpoints;
	if (midpoints.empty()) return combinedMidpoints;

	f32 start = midpoints[0];  // Start of the first group
	f32 end = start;           // End of the first group (to be updated)

	for (size_t i = 1; i < midpoints.size(); ++i) {
		f32 diff = std::abs(midpoints[i] - end);

		// Check if difference is within threshold or close due to circular wrap-around
		if (diff <= threshold || (1.0 - diff) <= threshold) {
			end = midpoints[i];  // Extend the range
		} else {
			// Calculate the circular midpoint for the current range
			float midpoint = (start + end) / 2;
			if (std::abs(start - end) > 0.5) {  // Wrap-around case
				midpoint = std::fmod((midpoint + 0.5), 1.0);  // Adjust for circular average
			}
			combinedMidpoints.push_back(midpoint);

			// Start a new range
			start = midpoints[i];
			end = start;
		}
	}

	// Add the final combined midpoint
	float finalMidpoint = (start + end) / 2;
	if (std::abs(start - end) > 0.5) {  // Wrap-around case
		finalMidpoint = std::fmod((finalMidpoint + 0.5), 1.0);
	}
	combinedMidpoints.push_back(finalMidpoint);

	return combinedMidpoints;
}

bool Classify::loadImage(std::string filePath) {
	ui8* loadedImage = stbi_load(filePath.c_str(), &m_imageSize.x, &m_imageSize.y, &m_imageChannelsCnt, 0);
	if (loadedImage) {
		deleteTexture(m_imageOriginalTextureID);
		deleteTexture(m_imagePreviewTextureID);

		if (m_imagePreview) {
			delete[] m_imagePreview;
			m_imagePreview = 0;
		}

		if (m_imageOriginal) {
			delete[] m_imageOriginal;
			m_imageOriginal = 0;
		}

		if (m_imageMask) {
			delete[] m_imageMask;
			m_imageMask = 0;
		}

		m_imageOriginal = loadedImage;

		i32 length = m_imageSize.x * m_imageSize.y * m_imageChannelsCnt;
		m_imagePreview = new ui8[length];
		m_imageMask = new ui8[length];
		std::memset(m_imageMask, 0, length * sizeof(ui8));
		std::memcpy(m_imagePreview, m_imageOriginal, length * sizeof(ui8));
		m_imagePreviewTextureID = createTexture(m_imageOriginal, m_imageSize.x, m_imageSize.y, m_imageChannelsCnt);
		m_imageOriginalTextureID = createTexture(m_imageOriginal, m_imageSize.x, m_imageSize.y, m_imageChannelsCnt);

		return true;
	} else {
		m_logger->addLog("Failed to load image: " + std::string(stbi_failure_reason()), ent::util::level::W);
		return false;
	}
}

void Classify::drawRectangularSectors() {
	if (!m_imagePreview) return; // Return if not initialized

	ui32 sectorHeight = std::max(m_imageSize.y / m_horizontalSectorsCnt, 1);
	ui32 sectorWidth = std::max(m_imageSize.x / m_verticalSectorsCnt, 1);

	f32 alpha = 0.4;
	for (int y = 0; y < m_imageSize.y; y += sectorHeight) {
		for (int x = 0; x < m_imageSize.x; x++) {
			ui32 index = y * m_imageSize.x * m_imageChannelsCnt + x * m_imageChannelsCnt;
			m_imagePreview[index + 0] = std::min(m_imagePreview[index + 0] + alpha * 255.0, 255.0);
			m_imagePreview[index + 1] = std::min(m_imagePreview[index + 0] - alpha * 255.0, 255.0);
			m_imagePreview[index + 2] = std::min(m_imagePreview[index + 0] - alpha * 255.0, 255.0);
		}
	}

	for (int x = 0; x < m_imageSize.x; x += sectorWidth) {
		for (int y = 0; y < m_imageSize.y; y++) {
			ui32 index = y * m_imageSize.x * m_imageChannelsCnt + x * m_imageChannelsCnt;
			m_imagePreview[index + 0] = std::min(m_imagePreview[index + 0] + alpha * 255.0, 255.0);
			m_imagePreview[index + 1] = std::min(m_imagePreview[index + 0] - alpha * 255.0, 255.0);
			m_imagePreview[index + 2] = std::min(m_imagePreview[index + 0] - alpha * 255.0, 255.0);
		}
	}
}

void Classify::drawSelection(i32v2 minPosition, i32v2 maxPosition) {
	if (!m_imageOriginal) return;

	ImVec2 currentMousePos = ImGui::GetMousePos();
	ImGuiIO& io = ImGui::GetIO();

	if (io.MouseDown[0]) { // LMB pressed. Select & render
		if (!m_isSelecting) {
			// Exit in case not clicked on image
			if (!ent::math::inRange(currentMousePos.x, minPosition.x, maxPosition.x)) return;
			if (!ent::math::inRange(currentMousePos.y, minPosition.y, maxPosition.y)) return;

			m_isSelecting = true;
			m_mouseStartPos = { std::min(std::max((i32)currentMousePos.x, minPosition.x), maxPosition.x), std::min(std::max((i32)currentMousePos.y, minPosition.y), maxPosition.y) };
		}

		m_mouseEndPos = { std::min(std::max((i32)currentMousePos.x, minPosition.x), maxPosition.x), std::min(std::max((i32)currentMousePos.y, minPosition.y), maxPosition.y) };

		f32v4 selectionColor = { 0,0,1,0.2 };
		drawRectangle(m_mouseStartPos, m_mouseEndPos - m_mouseStartPos, selectionColor, true, 0, true);
	} else if (m_isSelecting) {
		// Crop image
		m_isSelecting = false;
		if (io.MouseDown[0]) return; // Decline using RMB
		// Convert screen coords to image coords
		i32v2 imageOnScreenStart;
		i32v2 imageOnScreenSize;
		fitImageIntoRectangle(m_imageSize, { m_window.getWidth() / 2, m_window.getHeight() / 2 }, &imageOnScreenStart, &imageOnScreenSize);
		i32v2 imageOnScreenEnd = imageOnScreenStart + imageOnScreenSize;

		i32v2 startOnImage = { ent::math::mapValue(m_mouseStartPos.x, imageOnScreenStart.x, imageOnScreenEnd.x, 0, m_imageSize.x),
							   ent::math::mapValue(m_mouseStartPos.y, imageOnScreenStart.y, imageOnScreenEnd.y, 0, m_imageSize.y) };

		i32v2 endOnImage = { ent::math::mapValue(m_mouseEndPos.x, imageOnScreenStart.x, imageOnScreenEnd.x, 0, m_imageSize.x),
					   ent::math::mapValue(m_mouseEndPos.y, imageOnScreenStart.y, imageOnScreenEnd.y, 0, m_imageSize.y) };

		cropImage(startOnImage, endOnImage);
		m_needToUpdate = true;
	}
}

void Classify::updatePreview() {
	deleteTexture(m_imagePreviewTextureID);
	m_imagePreviewTextureID = createTexture(m_imagePreview, m_imageSize.x, m_imageSize.y, m_imageChannelsCnt);
}

void Classify::handlePerceptronProcessing() {
	if (!m_imageOriginal) return;
	if (m_needToUpdateAI) {
		m_needToUpdateAI = false;
		m_neuronsPerLayer = new i32[m_layersCount];
		m_neuronsPerLayer[0] = m_imageAISize.x * m_imageAISize.y;
		for (int i = 1; i < m_layersCount - 1; i++) {
			m_neuronsPerLayer[i] = 40;
		}
		m_neuronsPerLayer[m_layersCount - 1] = m_classesCount;
		m_perceptron.init(m_layersCount, m_neuronsPerLayer);
		m_perceptron.fillWeights();
	}

	i32 length = m_imageAISize.x * m_imageAISize.y;

	if (m_imageAI == 0) delete[] m_imageAI;
	m_imageAI = new f32[length];
	if (m_imageAIBW == 0) delete[] m_imageAIBW;
	m_imageAIBW = new ui8[length * 3];

	bool needToDisplayMask = m_displayMaskThreshold;
	m_displayMaskThreshold = false;
	handleMaskProcessing();

	for (i32 y = 0; y < m_imageAISize.y; y++) {
		for (i32 x = 0; x < m_imageAISize.x; x++) {
			ui32 indexAI = y * m_imageAISize.x + x;
			ui32 indexAIPreview = y * m_imageAISize.x * 3 + x * 3;

			ui32 xSource = ent::math::mapValue(x, 0, m_imageAISize.x - 1, 0, m_imageSize.x - 1);
			ui32 ySource = ent::math::mapValue(y, 0, m_imageAISize.y - 1, 0, m_imageSize.y - 1);
			ui32 indexSource = ySource * m_imageSize.x * m_imageChannelsCnt + xSource * m_imageChannelsCnt;

			if (m_imageMask[indexSource]) {
				f32v3 p = { m_imagePreview[indexSource + 0], m_imagePreview[indexSource + 1], m_imagePreview[indexSource + 2] };
				f32 avg = (p.r + p.g + p.b) / 765.0;
				m_imageAI[indexAI] = avg;
				m_imageAIBW[indexAIPreview + 0] = avg * 255.0;
				m_imageAIBW[indexAIPreview + 1]	= avg * 255.0;
				m_imageAIBW[indexAIPreview + 2] = avg * 255.0;
			} else {
				f32 avg = m_maskType == MaskType::eqOrLower ? 1 : 0;
				m_imageAI[indexAI] = avg;
				m_imageAIBW[indexAIPreview + 0] = avg * 255.0;
				m_imageAIBW[indexAIPreview + 1] = avg * 255.0;
				m_imageAIBW[indexAIPreview + 2] = avg * 255.0;
			}
		}
	}

	if (needToDisplayMask) {
		m_displayMaskThreshold = true;
	}

	deleteTexture(m_imageAITextureID);
	m_imageAITextureID = createTexture(m_imageAIBW, m_imageAISize.x, m_imageAISize.y, 3);
}

void Classify::processImageOptions() {
	for (i32 y = 0; y < m_imageSize.y; y++) {
		for (i32 x = 0; x < m_imageSize.x; x++) {
			ui32 index = y * m_imageSize.x * m_imageChannelsCnt + x * m_imageChannelsCnt;
			f32v3 p = { m_imageOriginal[index + 0], m_imageOriginal[index + 1] , m_imageOriginal[index + 2] };

			// Black and white
			f32 avg = (p.r + p.g + p.b) / 3.0;
			if (m_imageBlackAndWhite) {
				p = { avg, avg, avg };
			}

			// Brightness adjustment
			f32 brightnessBias = m_imageInvertColors ? -m_imageBrightness : m_imageBrightness;
			p.r = ent::math::clampValue(p.r + brightnessBias, 0, 255);
			p.g = ent::math::clampValue(p.g + brightnessBias, 0, 255);
			p.b = ent::math::clampValue(p.b + brightnessBias, 0, 255);

			// Contrast adjustment
			avg = (p.r + p.g + p.b) / 3.0;
			if (m_imageContrast > 0) { // Increase contrast
				p.r = avg >= 128 ? ent::math::mapValue(p.r, 128, 255, 128 + m_imageContrast, 255) : ent::math::mapValue(p.r, 0, 127, 0, 127 - m_imageContrast);
				p.g = avg >= 128 ? ent::math::mapValue(p.g, 128, 255, 128 + m_imageContrast, 255) : ent::math::mapValue(p.g, 0, 127, 0, 127 - m_imageContrast);
				p.b = avg >= 128 ? ent::math::mapValue(p.b, 128, 255, 128 + m_imageContrast, 255) : ent::math::mapValue(p.b, 0, 127, 0, 127 - m_imageContrast);

			} else { // Decrease Contrast
				p.r = avg >= 128 ? ent::math::mapValue(p.r, 128, 255, 128, 255 + m_imageContrast) : ent::math::mapValue(p.r, 0, 127, 0 - m_imageContrast, 127);
				p.g = avg >= 128 ? ent::math::mapValue(p.g, 128, 255, 128, 255 + m_imageContrast) : ent::math::mapValue(p.g, 0, 127, 0 - m_imageContrast, 127);
				p.b = avg >= 128 ? ent::math::mapValue(p.b, 128, 255, 128, 255 + m_imageContrast) : ent::math::mapValue(p.b, 0, 127, 0 - m_imageContrast, 127);
			}

			// Inverting colors;
			if (m_imageInvertColors) {
				p.r = 255 - p.r;
				p.g = 255 - p.g;
				p.b = 255 - p.b;
			}

			m_imagePreview[index + 0] = p.r;
			m_imagePreview[index + 1] = p.g;
			m_imagePreview[index + 2] = p.b;
		}
	}
}

void Classify::printToConsoleSectorsInfo() {
	std::cout << "\n\n\n\nCounted pixels per sector:\n";
	if (m_sectorsType == SectorType::rectangular) {
		for (i32 y = 0; y < m_horizontalSectorsCnt; y++) {
			for (i32 x = 0; x < m_verticalSectorsCnt; x++) {
				f32 pixelsCnt = m_countedPixelsPerSector[y * m_verticalSectorsCnt + x];
				std::cout << "[" << y << "][" << x << "]: " << pixelsCnt << "\n";
			}
		}
	}
	if (m_sectorsType == SectorType::circle) {
		for (i32 s = 0; s < m_circleSectorsCnt; s++) {
			f32 pixelsCnt = m_countedPixelsPerSector[s];
			std::cout << "[" << s << "]: " << pixelsCnt << "\n";
		}
	}
}

void Classify::processRectangularSectors() {
	if (!m_imageOriginal || !m_imagePreview) return; // Null check
	if (m_countedPixelsPerSector) delete[] m_countedPixelsPerSector;

	m_countedPixelsPerSector = new f32[m_horizontalSectorsCnt * m_verticalSectorsCnt];
	std::memset(m_countedPixelsPerSector, 0, m_horizontalSectorsCnt * m_verticalSectorsCnt * sizeof(f32));
	m_totalSectorsCountedPixels = 0;

	for (i32 y = 0; y < m_imageSize.y; y++) {
		for (i32 x = 0; x < m_imageSize.x; x++) {
			ui32 index = y * m_imageSize.x * m_imageChannelsCnt + x * m_imageChannelsCnt;
			f32v3 p = { m_imagePreview[index + 0], m_imagePreview[index + 1] , m_imagePreview[index + 2] };

			f32 avg = (p.r + p.g + p.b) / 3.0f;
			i32 sectorX = (x * m_verticalSectorsCnt) / m_imageSize.x;
			i32 sectorY = (y * m_horizontalSectorsCnt) / m_imageSize.y;
			i32 sectorIndex = sectorY * m_verticalSectorsCnt + sectorX;

			bool match = false;
			if (m_maskType == MaskType::eqOrLower && avg <= m_maskThreshold) {
				m_countedPixelsPerSector[sectorIndex]++;
				m_totalSectorsCountedPixels++;
				m_imageMask[index] = 1;
				match = true;
			}
			if (m_maskType == MaskType::eqOrHigher && avg >= m_maskThreshold) {
				m_countedPixelsPerSector[sectorIndex]++;
				m_totalSectorsCountedPixels++;
				m_imageMask[index] = 1;
				match = true;
			}
			if (m_maskType == MaskType::equal && avg == m_maskThreshold) {
				m_countedPixelsPerSector[sectorIndex]++;
				m_totalSectorsCountedPixels++;
				m_imageMask[index] = 1;
				match = true;
			}

			if (m_displayMaskThreshold && match) {
				m_imagePreview[index + 0] = (x + y % 2) % 2 ? 255 : 0;
				m_imagePreview[index + 1] = 0;
				m_imagePreview[index + 2] = (x + y % 2) % 2 ? 255 : 0;
			}
		}
	}			
}

void Classify::handleMaskProcessing() {
	for (i32 y = 0; y < m_imageSize.y; y++) {
		for (i32 x = 0; x < m_imageSize.x; x++) {
			ui32 index = y * m_imageSize.x * m_imageChannelsCnt + x * m_imageChannelsCnt;
			f32v3 p = { m_imagePreview[index + 0], m_imagePreview[index + 1] , m_imagePreview[index + 2] };

			f32 avg = (p.r + p.g + p.b) / 3.0f;
			bool match = false;

			if (m_maskType == MaskType::eqOrLower && avg <= m_maskThreshold) {
				m_imageMask[index] = 1;
				match = true;
			}
			if (m_maskType == MaskType::eqOrHigher && avg >= m_maskThreshold) {
				m_imageMask[index] = 1;
				match = true;
			}
			if (m_maskType == MaskType::equal && avg == m_maskThreshold) {
				m_imageMask[index] = 1;
				match = true;
			}

			if (m_displayMaskThreshold && match) {
				m_imagePreview[index + 0] = (x + y % 2) % 2 ? 255 : 0;
				m_imagePreview[index + 1] = 0;
				m_imagePreview[index + 2] = (x + y % 2) % 2 ? 255 : 0;
			}
		}
	}
}

void Classify::calculateNormalizedVector() {
	m_normalizedVector.clear();


	i32 length = 0;
	f32 divisor = 1;

	// Vector length
	if (m_sectorsType == SectorType::rectangular) {
		length = m_verticalSectorsCnt * m_horizontalSectorsCnt;
		m_normalizedVector.resize(length);
	
	} else if(m_sectorsType == SectorType::circle) {
		length = m_circleSectorsCnt;
		m_normalizedVector.resize(length);
	}

	// Normalization type
	if (m_vectorNormalizationType == VectorNormalization::normal) {
		for (i32 i = 0; i < length; i++) {
			divisor += m_countedPixelsPerSector[i];
		}
	} else if (m_vectorNormalizationType == VectorNormalization::mod) {
		for (i32 i = 0; i < length; i++) {
			divisor = m_countedPixelsPerSector[i] > divisor ? m_countedPixelsPerSector[i] : divisor;
		}
	}

	// Normalization
	for (i32 i = 0; i < length; i++) {
		m_normalizedVector[i] = m_countedPixelsPerSector[i] / divisor;
	}
}

void Classify::saveNormalizedVectorToFile() {
	const char* methodNames[] = { "MinMax",
								  "Average_distance",
								  "Angle",
								  "CNN",
								  "Recurrent_Hopfield_network" };

	std::string savePath = m_classifyFolderPath + std::string(methodNames[(i32)m_classifyMethod]);
	ent::io::createDirectory(ent::algorithm::stringToWstring(m_classifyFolderPath));
	ent::io::createDirectory(ent::algorithm::stringToWstring(savePath));

	// Vector length
	i32 length = 0;
	if (m_sectorsType == SectorType::rectangular) {
		length = m_verticalSectorsCnt * m_horizontalSectorsCnt;

	} else if (m_sectorsType == SectorType::circle) {
		length = m_circleSectorsCnt;
	}


	// Saving
	for (i32 class_ = 0; class_ < m_classesCount; class_++) {
		if (m_classesCheckboxes[class_]) {
			std::string classPath = savePath + "/" + std::to_string(class_);
			
			std::ofstream file;
			file.open(classPath, std::ios_base::app);
			file << std::fixed << std::setprecision(6);

			// Save shape
			if (m_classifyMethod == ClassifyMethod::averageDist || m_classifyMethod == ClassifyMethod::minMax) {
				if (m_sectorsType == SectorType::circle) {
					file << m_circleSectorsCnt << " : ";
				} else if (m_sectorsType == SectorType::rectangular) {
					file << m_verticalSectorsCnt << " " << m_horizontalSectorsCnt << " : ";
				}


				for (i32 i = 0; i < length; i++) {
					file << m_normalizedVector[i] << " ";
				}
				file << "\n";
			}

			if (m_classifyMethod == ClassifyMethod::angle) {
				f32 norm = m_anglesCount[0] + m_anglesCount[1] + m_anglesCount[2] + m_anglesCount[3];
				if (norm == 0) {
					file.close();
					continue;
				};
				if (m_vectorNormalizationType == VectorNormalization::mod) norm = std::max(std::max(m_anglesCount[0], m_anglesCount[1]), std::max(m_anglesCount[2], m_anglesCount[3]));
				float a = (f32)m_anglesCount[0] / norm;
				float b = (f32)m_anglesCount[1] / norm;
				float c = (f32)m_anglesCount[2] / norm;
				float d = (f32)m_anglesCount[3] / norm;
				file << a << " " << b << " " << c << " " << d << "\n";
			}
			file.close();
		}
	}

	// Printing to console
	if (m_needToLogToConsole) {
		std::cout << "\nNormalized vector: "; printToConsoleVectorInfo(m_normalizedVector);
	}
}

void Classify::saveAIImage() {
	const char* methodNames[] = { "MinMax",
								  "Average_distance",
								  "Angle",
								  "Perceptron",
								  "Genetic" };

	std::string savePath = m_classifyFolderPath + std::string(methodNames[(i32)m_classifyMethod]);
	ent::io::createDirectory(ent::algorithm::stringToWstring(m_classifyFolderPath));
	ent::io::createDirectory(ent::algorithm::stringToWstring(savePath));

	if (m_needToLogToConsole) std::cout << "\nAI image:\n";
	// Saving
	for (i32 class_ = 0; class_ < m_classesCount; class_++) {
		if (m_classesCheckboxes[class_]) {
			std::string classPath = savePath + "/" + std::to_string(class_);

			std::ofstream file;
			file.open(classPath, std::ios_base::app);
			file << std::fixed << std::setprecision(6);

			i32 selectedCheckbox = 0;
			for (int i = 0; i < m_classesCount; i++) {
				if (m_classesCheckboxes[i]) selectedCheckbox = i;
			}

			file << m_imageAISize.x << " " << m_imageAISize.y << " " << selectedCheckbox << "\n";

			// Save shape
			for (i32 y = 0; y < m_imageAISize.y; y++) {
				for (i32 x = 0; x < m_imageAISize.x; x++) {
					ui32 index = y * m_imageAISize.x + x;
					file << (f32)m_imageAI[index] << "\t";
					if (m_needToLogToConsole) std::cout << (f32)m_imageAI[index] << "\t";
				}
				file << "\n";
				if (m_needToLogToConsole) std::cout << "\n";
			}
			file.close();
		}
	}
}

void Classify::printToConsoleVectorInfo(std::vector<f32>& vector) {
	std::cout << "[";
	for (int i = 0; i < vector.size(); i++) {
		std::cout << vector[i] << ((i != vector.size() - 1) ? ", " : "]\n");
	}
}

void Classify::classifyImage() {
	if (!m_imageOriginal) return; // Null check
	clearCheckboxes();

	std::vector<f32> minVector;
	std::vector<f32> maxVector;
	std::vector<f32> avgVector;
	std::vector<f32> matchVector;
	f32 maxMatch = 0;

	// Used in Angle method for normalization (by default not normalized)
	f32 angleNorm = m_anglesCount[0] + m_anglesCount[1] + m_anglesCount[2] + m_anglesCount[3];
	if (m_vectorNormalizationType == VectorNormalization::mod) angleNorm = std::max(std::max(m_anglesCount[0], m_anglesCount[1]), std::max(m_anglesCount[2], m_anglesCount[3]));

	matchVector.resize(m_classesCount, -1);

	// Vector length
	i32 length = 0;
	if (m_sectorsType == SectorType::rectangular) {
		length = m_verticalSectorsCnt * m_horizontalSectorsCnt;

	} else if (m_sectorsType == SectorType::circle) {
		length = m_circleSectorsCnt;
	}
	if (m_classifyMethod == ClassifyMethod::angle) {
		length = 4;
	}
	if (m_classifyMethod == ClassifyMethod::perceptron || m_classifyMethod == ClassifyMethod::genetic) {
		std::vector<f64> input;
		length = m_imageAISize.x * m_imageAISize.y;
		input.resize(length);
			
		for (int i = 0; i < length; i++) {
			input[i] = m_imageAI[i];
		}
		m_perceptron.fillInputVector(input);
		m_perceptron.forwardPropagation();

		for (int i = 0; i < m_classesCount; i++) {
			matchVector[i] = m_perceptron.layer[m_perceptron.layersCount - 1][i];
		}
	} else { 

		// Iterate over each type and calculate vectors
		if (angleNorm) {
			for (i32 class_ = 0; class_ < m_classesCount; class_++) {
				minVector = std::vector<f32>(length, 1);
				maxVector = std::vector<f32>(length, 0);
				avgVector = std::vector<f32>(length, 0);
				if (!fillMinMaxAvgVectors(minVector, maxVector, avgVector, class_)) continue; // Skip if vectors not filled
				matchVector[class_] = 0;


				// Iterate over normalized vector and compare
				for (i32 i = 0; i < length; i++) {
					if (m_classifyMethod == ClassifyMethod::minMax) {
						if (m_normalizedVector[i] <= maxVector[i] && m_normalizedVector[i] >= minVector[i]) matchVector[class_] += 1.0 / length;
					}

					if (m_classifyMethod == ClassifyMethod::averageDist) {
						f32 a = avgVector[i];
						f32 b = m_normalizedVector[i];

						matchVector[class_] += std::max(a, b) - std::min(a, b);
					}

					if (m_classifyMethod == ClassifyMethod::angle) {
						f32 a = avgVector[i];
						f32 b = (f32)m_anglesCount[i] / angleNorm;

						matchVector[class_] += std::max(a, b) - std::min(a, b);
					}
				}

				if (m_needToLogToConsole) {
					std::cout << "\n\nClass [" << class_ << "] parameters:";
					std::cout << "\nSample MinVector: "; 	printToConsoleVectorInfo(minVector);
					std::cout << "\nSample MaxVector: "; 	printToConsoleVectorInfo(maxVector);
					std::cout << "\nSample AvgVector: "; 	printToConsoleVectorInfo(avgVector);
					std::cout << "\n\n";
				}
			}
		}
	}


	// Iterate over vectors and select best
	f32 maxClass = 0;
	f32 minClass = 1000000;
	f32 bestClass = 0;
	for (i32 class_ = 0; class_ < m_classesCount; class_++) {
		if (matchVector[class_] != -1) { // Skip missing classes
			if (m_classifyMethod == ClassifyMethod::averageDist || m_classifyMethod == ClassifyMethod::angle) { // Best - min value (Average distance)
				if (matchVector[class_] < minClass) {
					minClass = matchVector[class_];
					bestClass = minClass;
				}
			} else { // Best - max value (MinMax method)
				if (matchVector[class_] > maxClass) {
					maxClass = matchVector[class_];
					bestClass = maxClass;
				}
			}
		}
	}

	if (m_needToLogToConsole) {
		if (m_classifyMethod == ClassifyMethod::angle) {
			std::cout << "\n\n\nSample curVector: ";
			f32 a = (f32)m_anglesCount[0] / angleNorm;
			f32 b = (f32)m_anglesCount[1] / angleNorm;
			f32 c = (f32)m_anglesCount[2] / angleNorm;
			f32 d = (f32)m_anglesCount[3] / angleNorm;
			std::cout << a << " " << b << " " << c << " " << d << "\n";
		} else if(m_classifyMethod != ClassifyMethod::perceptron && m_classifyMethod != ClassifyMethod::genetic){
			std::cout << "\n\n\nSample curVector: "; 	printToConsoleVectorInfo(m_normalizedVector);
		}
		std::cout << "\n\nClass prediction:\n";
		std::cout << bestClass << "\n";
	}

	bool hasClassification = false;
	// Log to console & apply on checkboxes
	for (i32 class_ = 0; class_ < m_classesCount; class_++) {
		if (matchVector[class_] != -1) { 
			if (m_needToLogToConsole) std::cout << "Class [" << class_ << "]: " << matchVector[class_] << ((matchVector[class_] == bestClass) ? " best\n" : "\n");
			m_classesCheckboxes[class_] = matchVector[class_] == bestClass;
			hasClassification = true;
		}
	}

	// If no match
	if (!hasClassification) std::cout << "\n\n\nNo class matches this image!\nTry selection other method/normalization/sector type.\nAlso inspect data folder!\n";
	if (m_needToLogToConsole) std::cout << "\n\n\n";
	
}

void Classify::clearCheckboxes() {
	for (int i = 0; i < m_classesCount; i++) {
		m_classesCheckboxes[i] = 0;
	}
}

void Classify::processCircleSectors() {
	if (!m_imageOriginal || !m_imagePreview) return; // Null check
	if (m_countedPixelsPerSector) delete[] m_countedPixelsPerSector;

	m_countedPixelsPerSector = new f32[m_circleSectorsCnt];
	std::memset(m_countedPixelsPerSector, 0, m_circleSectorsCnt * sizeof(f32));
	m_totalSectorsCountedPixels = 0;

	i32 originalSeed = rand();
	srand(131331);

	f32 alpha = 1.6;
	f32 minAlpha = 0.4;

	f32v3 vector = { 1, 0, 0 };
	f32v3 vectorNext = { 1, 0, 0 };
	f32v3 vectorDiagonal = { -m_imageSize.x, -m_imageSize.y, 0 };
	f32v3 temp;

	for (i32 s = 0; s < m_circleSectorsCnt; s++) {
		temp = vectorDiagonal * (f32)s * (1.0f / m_circleSectorsCnt);
		vector = glm::normalize(f32v3(temp.x + m_imageSize.x, -temp.y, 0));
		temp = vectorDiagonal * ((f32)s + 1.0f) * (1.0f / m_circleSectorsCnt);
		vectorNext = glm::normalize(f32v3(temp.x + m_imageSize.x, -temp.y, 0));

		f32v3 color = glm::normalize(f32v3(rand(), rand(), rand()));

		for (i32 y = 0; y < m_imageSize.y; y++) {
			for (i32 x = 0; x < m_imageSize.x; x++) {
				ui32 index = y * m_imageSize.x * m_imageChannelsCnt + x * m_imageChannelsCnt;

				f32v3 point = glm::normalize(f32v3(x, y, 0));
				f32 direction = glm::cross(vector, point).z;
				f32 directionNext = glm::cross(vectorNext, point).z;

				if (direction > 0 && directionNext <= 0) {
					f32v3 p = { m_imagePreview[index + 0], m_imagePreview[index + 1] , m_imagePreview[index + 2] };
					f32 avg = (p.r + p.g + p.b) / 3.0f;
					i32 sectorIndex = s;

					// Count pixels in sectors
					bool match = false;
					if (m_maskType == MaskType::eqOrLower && avg <= m_maskThreshold) {
						m_countedPixelsPerSector[sectorIndex]++;
						m_totalSectorsCountedPixels++;
						m_imageMask[index] = 1;
						match = true;
					}
					if (m_maskType == MaskType::eqOrHigher && avg >= m_maskThreshold) {
						m_countedPixelsPerSector[sectorIndex]++;
						m_totalSectorsCountedPixels++;
						m_imageMask[index] = 1;
						match = true;
					}
					if (m_maskType == MaskType::equal && avg == m_maskThreshold) {
						m_countedPixelsPerSector[sectorIndex]++;
						m_totalSectorsCountedPixels++;
						m_imageMask[index] = 1;
						match = true;
					}

					// Display mask
					if (m_displayMaskThreshold && match) {
						m_imagePreview[index + 0] = (x + y % 2) % 2 ? 255 : 0;
						m_imagePreview[index + 1] = 0;
						m_imagePreview[index + 2] = (x + y % 2) % 2 ? 255 : 0;
					}

					// Display sectors
					if (m_displaySectors) {
						m_imagePreview[index + 0] = ent::math::clampValue(m_imagePreview[index + 0] * color.r * alpha + color.r * minAlpha * 255.0, 0, 255);
						m_imagePreview[index + 1] = ent::math::clampValue(m_imagePreview[index + 1] * color.g * alpha + color.g * minAlpha * 255.0, 0, 255);
						m_imagePreview[index + 2] = ent::math::clampValue(m_imagePreview[index + 2] * color.b * alpha + color.b * minAlpha * 255.0, 0, 255);
					}
				}
			}
		}
	}
}

void Classify::handleAutoCrop() {
	if (!m_needToAutoCrop) return; // Return if no need to autoCrop
	m_needToAutoCrop = false;

	// Find min/max mask positions
	i32v2 topLeft = { m_imageSize.x, m_imageSize.y };
	i32v2 bottomRight = { 0, 0 };
	bool needToCrop = false;

	for (i32 y = 0; y < m_imageSize.y; y++) {
		for (i32 x = 0; x < m_imageSize.x; x++) {
			i32 index = y * m_imageSize.x * m_imageChannelsCnt + x * m_imageChannelsCnt;
			if (m_imageMask[index]) { // If there is mask on this position;
				if (x < topLeft.x) topLeft.x = x;
				if (x > bottomRight.x) bottomRight.x = x;
				if (y < topLeft.y) topLeft.y = y;
				if (y > bottomRight.y) bottomRight.y = y;
				needToCrop = true;
			}
		}
	}

	if (!needToCrop) return; // Return if no need to crop

	i32 width = bottomRight.x - topLeft.x;
	i32 height = bottomRight.y - topLeft.y;
	i32 length = width > height ? width : height;

	// Make selection to be rectangle like
	if (m_autoCropRectangular) {
		if (width > height) {
			topLeft.y -= (width - height) / 2;
			bottomRight.y += (width - height) / 2;
		}
		if (height > width) {
			topLeft.x -= (height - width) / 2;
			bottomRight.x += (height - width) / 2;
		}
	}

	// Add padding
	i32 padding = length * m_autoCropPadding;
	topLeft -= padding;
	bottomRight += padding;

	cropImage(topLeft, bottomRight);
}

void Classify::trainAI() {
	clearCheckboxes();

	if (!m_AITrainData.size()) {
		if (!fillAITrainData()) {
			std::cout << "Train data is empty!\n";
			m_needToTrainAI = false;
			return;
		}
	}

	auto batch = selectBatch(m_AITrainData, m_batchSize);
	if (m_needToLogToConsole) {
		std::cout << "Batch [" << batch.size() << "]:\n";
	}
	for (int i = 0; i < batch.size(); i++) {
		if (m_needToLogToConsole) {
			for (int j = 0; j < batch[i].second.size(); j++) {
				if (batch[i].second[j]) std::cout << "   image [" << i << "]: " << j << "\n";
			}
		}
		m_perceptron.fillInputVector(batch[i].first);
		m_perceptron.fillExpectedVector(batch[i].second);
		m_perceptron.forwardPropagation();
		m_perceptron.updateAllGradients();
	}
	m_perceptron.applyAllGradients(m_learningRate);
	m_aiError = m_perceptron.totalCost();
	
	f32 maxValue = 0;
	i32 maxIndex = 0;
	// Log to console & apply on checkboxes
	if (m_needToLogToConsole)  std::cout << "\n\nClass prediction:\n";
	for (i32 class__ = 0; class__ < m_classesCount; class__++) {
		if (m_perceptron.layer[m_perceptron.layersCount - 1][class__] > maxValue) {
			maxValue = m_perceptron.layer[m_perceptron.layersCount - 1][class__];
			maxIndex = class__;
		}
		if (m_needToLogToConsole) std::cout << "Class [" << class__ << "]: " << m_perceptron.layer[m_perceptron.layersCount - 1][class__] << "   " << m_perceptron.trainingLayer[class__] << "\n";
	}

	m_classesCheckboxes[maxIndex] = true;

	if (m_needToLogToConsole) std::cout << "\n\n\n";
}

void Classify::loadWeights() {
	std::cout << "Loading weights\n";
	m_weightsFileName = std::string("/perceptron_") + std::to_string(m_imageAISize.x) + "_" + std::to_string(m_imageAISize.y) + "__";
	for (int i = 0; i < m_layersCount; i++) {
		m_weightsFileName += std::to_string(m_neuronsPerLayer[i]) + "_";
	}

	const char* methodNames[] = { "MinMax",
								  "Average_distance",
								  "Angle",
								  "Perceptron",
								  "Genetic" };

	std::string savePath = m_classifyFolderPath + std::string(methodNames[(i32)m_classifyMethod]);
	ent::io::createDirectory(ent::algorithm::stringToWstring(m_classifyFolderPath));
	ent::io::createDirectory(ent::algorithm::stringToWstring(savePath));

	savePath += m_weightsFileName;
	m_perceptron.loadWeights(savePath.c_str());
}

void Classify::saveWeights() {
	std::cout << "Saving weights\n";
	m_weightsFileName = std::string("/perceptron_") + std::to_string(m_imageAISize.x) + "_" + std::to_string(m_imageAISize.y) + "__";
	for (int i = 0; i < m_layersCount; i++) {
		m_weightsFileName += std::to_string(m_neuronsPerLayer[i]) + "_";
	}

	const char* methodNames[] = { "MinMax",
								  "Average_distance",
								  "Angle",
								  "Perceptron",
								  "Genetic" };

	std::string savePath = m_classifyFolderPath + std::string(methodNames[(i32)m_classifyMethod]);
	ent::io::createDirectory(ent::algorithm::stringToWstring(m_classifyFolderPath));
	ent::io::createDirectory(ent::algorithm::stringToWstring(savePath));

	savePath += m_weightsFileName;
	m_perceptron.saveWeights(savePath.c_str());
}

std::vector<std::pair<std::vector<double>, std::vector<double>>> Classify::selectBatch(std::vector<std::pair<std::vector<double>, std::vector<double>>>& data, size_t batchSize) {
	std::random_device rd;
	std::mt19937 gen(rd());
	std::shuffle(data.begin(), data.end(), gen);

	std::vector<std::pair<std::vector<double>, std::vector<double>>> batch;
	for (size_t i = 0; i < batchSize && i < data.size(); ++i) {
		batch.push_back(data[i]);
	}

	return batch;
}

bool Classify::fillAITrainData() {
	const char* methodNames[] = { "MinMax",
							  "Average_distance",
							  "Angle",
							  "Perceptron",
							  "Genetic" };

	std::string savePath = m_classifyFolderPath + std::string(methodNames[(i32)m_classifyMethod]);
	bool hasAtLeastOneMatch = false;

	for (i32 class_ = 0; class_ < m_classesCount; class_++) {
		std::string filePath = savePath + "/" + std::to_string(class_);

		std::ifstream file;
		file.open(filePath);
		i32 curClassType = 0;
		std::string line;

		while (std::getline(file, line)) {
			std::stringstream ss(line);

			std::string firstParameter = "";
			std::string secondParameter = "";
			std::string thirdParameter = "";
			i32 width = 0;
			i32 height = 0;
			i32 length = 0;

			std::pair<std::vector<f64>, std::vector<f64>> data;

			ss >> firstParameter;
			ss >> secondParameter;
			ss >> thirdParameter;


			width = std::stoi(firstParameter);
			height = std::stoi(secondParameter);
			length = width * height;

			if (width != m_imageAISize.x || height != m_imageAISize.y) {
				continue;
			}

			curClassType = std::stoi(thirdParameter);

			data.first.resize(length, 0);
			data.second.resize(m_classesCount, 0);

			// Expected
			for (int i = 0; i < data.second.size(); i++) {
				data.second[i] = 0;
			}
			data.second[curClassType] = 1;

			// Input
			for (int y = 0; y < height; y++) {
				std::getline(file, line);
				std::stringstream sss(line);
	
				// Filling vectors
				for (i32 x = 0; x < width; x++) {
					f32 value = 0;
					sss >> value;
					data.first[y * width + x] = value;
				}
			}

			m_AITrainData.push_back(data);
			hasAtLeastOneMatch = true;

		}
		file.close();
	}

	return hasAtLeastOneMatch;
}

void Classify::cropImage(i32v2 minPosition, i32v2 maxPosition) {
	if (!m_imageOriginal || !m_imagePreview) return; // Null check

	i32v2 topLeft = { std::min(minPosition.x, maxPosition.x), std::min(minPosition.y, maxPosition.y) };
	i32v2 bottomRight = { std::max(minPosition.x, maxPosition.x), std::max(minPosition.y, maxPosition.y) };

	if (!(abs(bottomRight.x) - abs(topLeft.x)) || !(abs(bottomRight.y) - abs(topLeft.y))) return; // In case it is smaller than 1px in any dimension return

	bottomRight.x++;
	bottomRight.y++;

	ui32 croppedWidth = bottomRight.x - topLeft.x;
	ui32 croppedHeight = bottomRight.y - topLeft.y;
	ui8* croppedImage = new ui8[croppedWidth * croppedHeight * m_imageChannelsCnt];

	for (i32 y = 0; y < croppedHeight; y++) {
		for (i32 x = 0; x < croppedWidth; x++) {
			ui32 croppedIndex = y * croppedWidth * m_imageChannelsCnt + x * m_imageChannelsCnt;
			ui32 imageIndex = topLeft.y * m_imageSize.x * m_imageChannelsCnt + topLeft.x * m_imageChannelsCnt + y * m_imageSize.x * m_imageChannelsCnt + x * m_imageChannelsCnt;

			// In case outOfBounds - fill with grey color
			if (topLeft.x + x < 0 || topLeft.y + y < 0 || topLeft.x + x >= m_imageSize.x || topLeft.y + y >= m_imageSize.y) {
				croppedImage[croppedIndex + 0] = m_autoCropFillColor.r * 255.0;
				croppedImage[croppedIndex + 1] = m_autoCropFillColor.g * 255.0;
				croppedImage[croppedIndex + 2] = m_autoCropFillColor.b * 255.0;
				if (m_imageChannelsCnt == 4) croppedImage[croppedIndex + 3] = m_autoCropFillColor.a * 255.0;
				continue;
			}

			// Copy from source
			croppedImage[croppedIndex + 0] = m_imageOriginal[imageIndex + 0];
			croppedImage[croppedIndex + 1] = m_imageOriginal[imageIndex + 1];
			croppedImage[croppedIndex + 2] = m_imageOriginal[imageIndex + 2];
			if(m_imageChannelsCnt == 4) croppedImage[croppedIndex + 3] = m_imageOriginal[imageIndex + 3];
		}
	}

	m_imageSize.x = croppedWidth;
	m_imageSize.y = croppedHeight;

	delete[] m_imageOriginal;
	delete[] m_imagePreview;
	delete[] m_imageMask;
	
	i32 length = m_imageSize.x * m_imageSize.y * m_imageChannelsCnt;
	m_imageOriginal = croppedImage;
	m_imagePreview = new ui8[length * sizeof(ui8)];
	m_imageMask = new ui8[length * sizeof(ui8)];
	std::memcpy(m_imagePreview, m_imageOriginal, length * sizeof(ui8));
	std::memset(m_imageMask, 0, length * sizeof(ui8));

	deleteTexture(m_imageOriginalTextureID);
	deleteTexture(m_imagePreviewTextureID);

	m_imageOriginalTextureID = createTexture(m_imageOriginal, m_imageSize.x, m_imageSize.y, m_imageChannelsCnt);
	m_imagePreviewTextureID = createTexture(m_imagePreview, m_imageSize.x, m_imageSize.y, m_imageChannelsCnt);
}

void Classify::selectAutoCropFillColor() {
	if (!m_needToSelectAutoCropColor) return; // Return in case if no need
	if (timer.active()) return; // Return if timer still goes

	ImGuiIO io = ImGui::GetIO();

	if (io.MouseDown[0]) {
		ImVec2 mousePos = ImGui::GetMousePos();
		std::cout << mousePos.x << " " << mousePos.y << "\n";
		ui8 pixel[3];
		glReadBuffer(GL_BACK);

		glReadPixels(mousePos.x, io.DisplaySize.y - mousePos.y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);
		std::cout << (i32)pixel[0] << " " << (i32)pixel[1] << " " <<  (i32)pixel[2] << "\n";

		m_autoCropFillColor.r = (f32)pixel[0] / 255.0;
		m_autoCropFillColor.g = (f32)pixel[1] / 255.0;
		m_autoCropFillColor.b = (f32)pixel[2] / 255.0;
		m_autoCropFillColor.a = 1;
		m_needToSelectAutoCropColor = false;
	}
}

bool Classify::fillMinMaxAvgVectors(std::vector<f32>& minVector, std::vector<f32>& maxVector, std::vector<f32>& avgVector, i32 classType) {
	const char* methodNames[] = { "MinMax",
							  "Average_distance",
							  "Angle",
							  "Perceptron",
							  "Genetic" };

	std::string savePath = m_classifyFolderPath + std::string(methodNames[(i32)m_classifyMethod]);

	std::ifstream file;
	std::string filePath = savePath + "/" + std::to_string(classType);
	bool hasAtLeastOneMatch = false;
	i32 matchCnt = 0;

	file.open(filePath);
	if (!file.is_open()) return false;
 

	std::string line;
	while (std::getline(file, line)) {
		std::stringstream ss(line);

		// Extract sector type and shape
		SectorType currentSectorType = SectorType::rectangular;
		f32 currentVectorSum = 0;
		VectorNormalization currentNormalizationType = VectorNormalization::normal;
		std::vector<f32> tmpVector;
		std::string firstParameter = "";
		std::string secondParameter = "";
		i32 verticalSectors = 0;
		i32 horizontalSectors = 0;
		i32 circleSectors = 0;
		i32 length = 0;


		if (m_classifyMethod != ClassifyMethod::angle) {
			ss >> firstParameter;
			ss >> secondParameter;

			if (secondParameter[0] == ':') {
				currentSectorType = SectorType::circle;
				circleSectors = std::stoi(firstParameter);
				length = circleSectors;
			} else {
				verticalSectors = std::stoi(firstParameter);
				horizontalSectors = std::stoi(secondParameter);
				length = verticalSectors * horizontalSectors;
				ss >> secondParameter;
			}


			// Skip line in case data differs
			if (currentSectorType != m_sectorsType) continue;
			if (m_sectorsType == SectorType::rectangular) {
				if (verticalSectors != m_verticalSectorsCnt) continue;
				if (horizontalSectors != m_horizontalSectorsCnt) continue;
			}
			if (m_sectorsType == SectorType::circle) {
				if (circleSectors != m_circleSectorsCnt) continue;
			}
		} else {
			length = 4;
		}
		tmpVector.resize(length, 0);

		// Filling vectors
		for (i32 s = 0; s < length; s++) {
			f32 value = 0;
			ss >> value;
			currentVectorSum += value;
			tmpVector[s] = value;

			if (value < minVector[s]) minVector[s] = value; // Min
			if (value > maxVector[s]) maxVector[s] = value; // Max
		}

		currentNormalizationType = currentVectorSum > 1.00001 ? VectorNormalization::mod : VectorNormalization::normal;
		if (currentNormalizationType != m_vectorNormalizationType) continue;
		
		// Average vector
		for (i32 s = 0; s < length; s++) {
			avgVector[s] += tmpVector[s];
		}

		matchCnt++;
		hasAtLeastOneMatch = true;
	}
	file.close();

	i32 length = m_sectorsType == SectorType::rectangular ? m_verticalSectorsCnt * m_horizontalSectorsCnt : m_circleSectorsCnt;
	if (m_classifyMethod == ClassifyMethod::angle) length = 4;

	for (i32 s = 0; s < length; s++) {
		avgVector[s] /= matchCnt;
	}

	return hasAtLeastOneMatch;
}
