#pragma once
#include "../UI/GameWindow.h"
#include "../Utility/Timer.h"
#include "ClassifyOptions.h"
#include "../AI/NeuralNetwork.h"
#include <vector>


class Classify {
public:
	Classify();

	void runLoop();
private:
	i32 windowWidth = 900;
	i32 windowHeight = 900;
	i32 imagePathBufferSize = 4096;
	i32 fontSize = 18;
	i32 paddingSmall = 4;
	i32 paddingMedium = 12;
	i32 paddingBig = 24;
	i32 buttonHeightSmall = 30;
	i32 buttonHeightMedium = 40;
	f32v4 defaultTextColor = { 1, 1, 1, 1 };

	ent::ui::GameWindow m_window;
	ClassifyMethod m_classifyMethod;
	MaskType m_maskType;
	std::string m_imagePath;
	ent::util::Logger* m_logger;

	i32v2 m_imageSize;
	i32 m_imageChannelsCnt;
	ui8* m_imageOriginal;
	ui8* m_imagePreview;
	ui8* m_imageMask;

	i32 m_imageOriginalTextureID;
	i32 m_imagePreviewTextureID;

	f32 m_imageBrightness;
	f32 m_imageContrast;
	bool m_imageBlackAndWhite;
	bool m_imageInvertColors;

	bool m_needToUpdate;

	bool m_displayMaskThreshold;
	i32 m_maskThreshold;

	SectorType m_sectorsType;
	bool m_displaySectors;
	i32 m_circleSectorsCnt;
	i32 m_horizontalSectorsCnt;
	i32 m_verticalSectorsCnt;
	i32 m_totalSectorsCountedPixels;
	f32* m_countedPixelsPerSector;

	i32 m_classesCount;
	bool* m_classesCheckboxes;

	i32v2 m_mouseStartPos;
	i32v2 m_mouseEndPos;
	bool m_isSelecting;


	std::string m_classifyFolderPath;
	bool m_needToClassify;
	bool m_needToSave;
	bool m_needToLogToConsole;
	VectorNormalization m_vectorNormalizationType;
	std::vector<f32> m_normalizedVector;

	bool m_needToAutoCrop;
	bool m_needToSelectAutoCropColor;
	bool m_autoCropRectangular;
	f32 m_autoCropPadding;
	f32v4 m_autoCropFillColor;
	ent::util::Timer timer;


	f32 m_circleOffset;
	f32 m_circleRadius;
	f32 m_CombineAngleThreshold;
	i32 m_circlePrecision;
	i32 m_maxIter;
	i32 m_lerpIter;
	bool m_displayAngleAux;
	bool m_displayRectangles;
	std::vector<i32> m_anglesCount;


	NeuralNetwork m_perceptron;
	i32 m_layersCount;
	i32* m_neuronsPerLayer;
	i32 m_batchSize;
	f32 m_learningRate;

	i32v2 m_imageAISize;
	f32* m_imageAI;
	ui8* m_imageAIBW;
	i32 m_imageAITextureID;
	f32 m_aiAccuracyTarget;
	f32 m_aiError;
	std::string m_weightsFileName;

	bool m_needToUpdateAI;
	bool m_needToTrainAI;
	bool m_needToSaveWeights;
	bool m_needToLoadWeights;

	ui32 m_epochSize;

	std::vector<std::pair<std::vector<f64>, std::vector<f64>>> m_AITrainData;



	void handleUI();
	void handleProcessing();


	///////// UI elements //////////////////////////////////////////////////
	void beginControlWindow(i32v2 position, i32v2 shape);
	void endControlWindow();

	// Handles image opening UI
	void handleImageOpen();

	// Handles image auto cropping UI
	void handleImageAutoCrop();

	// Handles image opening UI
	void handleClassifyMethodSelection();
	void handleImageSectorProcessing();
	void handleImageProcessing();
	void handleImageSelection();
	void handleMaskProcessing();

	void handleMinMaxMethod();
	void handleAverageDistMethod();
	void handleAngleMethod();
	void handlePerceptronMethod();
	void handleGeneticMethod();

	void handleRecursiveAngleMethod(i32v2 centerPoint = { -1,-1 }, int n = 5);
	std::vector<i32v2> handleAngleMethodProcessing(i32v2 centerPoint = {-1,-1});

	std::vector<f32> combineMidpoints(const std::vector<f32>& midpoints, f32 threshold);

	void handlePerceptronProcessing();

	/*
	*  Moves current IMGUI cursor by a given pixels amount
	*  If applyToWindow is set to true applies to the next Window
	*/ 
	void imguiPadding(i32 x = 0, i32 y = 0);



	///////// UI helping elements //////////////////////////////////////////
	// Creates OpenGL texture ID 
	ui32 createTexture(ui8* data, i32 width, i32 height, i32 nChannels);

	// Deletes OpenGL texture
	void deleteTexture(ui32 textureID);


	// Adds circle drawCall using IMGUI
	void drawCircle(i32v2& center, f32 radius, f32v4 color, bool filled = false, bool alwaysOnTop = false, float thickness = 1.0f, int segments = 64);   

	// Adds rectangle drawCall using IMGUI
	void drawRectangle(i32v2 position, i32v2 size, f32v4 color, bool filled = false, float outlineThickness = 0.0f, bool alwaysOnTop = false);

	// Adds text pattern drawCall using IMGUI
	void drawPattern(std::string pattern, f32v4 color = { 1,1,1,1 });

	// Adds image drawCall using IMGUI
	void drawImage(i32v2 windowPosition, i32v2 windowSize, i32 imageTextureId, i32v2 imageSize);

	void setTextColor(f32v4 color = { 1, 1, 1, 1 });
	void endTextColor();
	/* 
	*  Calculates position and dimensions of an image inside some rectangle saving image aspect ratio
	* 
	*  Input: imageSize - size of the image
	*         rectSize  - size of the area you want to fit image
	*  
	*  Output: outPosition - top left corner position for the image relative to the top left of rectSize
	*          outSize     - size of the image inside rectangle
	*/
	void fitImageIntoRectangle(i32v2 imageSize, i32v2 rectSize, i32v2* outPosition, i32v2* outSize);

	void drawSelection(i32v2 minPosition, i32v2 maxPosition);


	// Creates new OpenGL texture ID for preview image
	void updatePreview();

	// Crops image
	void cropImage(i32v2 minPosition, i32v2 maxPosition);

	bool loadImage(std::string filePath);


	///////// Calculation helping elements //////////////////////////////////////////
	void drawRectangularSectors();
	void processImageOptions();
	void processRectangularSectors();
	void processCircleSectors();

	void printToConsoleSectorsInfo();
	void printToConsoleVectorInfo(std::vector<f32>& vector);
	void calculateNormalizedVector();
	void saveNormalizedVectorToFile();

	void saveAIImage();
	void saveWeights();
	void loadWeights();
	void trainAI();

	void classifyImage();

	// Fills vectors with data. Returns true if at least one case parameters match our current
	bool fillMinMaxAvgVectors(std::vector<f32>& minVector, std::vector<f32>& maxVector, std::vector<f32>& avgVector, i32 classType);

	bool fillAITrainData();
	std::vector<std::pair<std::vector<double>, std::vector<double>>> selectBatch(std::vector<std::pair<std::vector<double>, std::vector<double>>>& data, size_t batchSize);

	void handleAutoCrop();

	void selectAutoCropFillColor();

	void clearCheckboxes();
};