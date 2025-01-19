#pragma once
#include <vector>

class Layer {
public:
	Layer();
	~Layer();

	int neuronsCount;
	int weightsCount;

	double* neurons;
	double* weights;
	double* biasWeights;
	double bias;

	double* nodeValues;
	double* gradients;
	double* biasGradients;

	bool init(int neuronsCount, int neuronsCountInPreviousLayer);
	void clearNeurons();
	void clearNodes();
	void clearGradients();
	void clearWeights();

	double& operator[] (int index);
};



class NeuralNetwork {
public:
	NeuralNetwork();
	NeuralNetwork(int layersCount, int neuronsInLayerCount[]);
	~NeuralNetwork();

	Layer* layer;
	Layer trainingLayer;

	int layersCount;
	bool outputDebugInfo;
	bool initialized;

	void init(int layersCount, int neuronsInLayerCount[]);

	void forwardPropagation(std::vector<double> inputVector = {});
	void updateAllGradients();
	void applyAllGradients(double learningRate);

	void calculateLayer(Layer& input, Layer& target);
	void updateGradients(Layer& target, Layer& previousLayer);
	void applyGradients(Layer& target, double learningRate);
	void calculateOutputLayerNodeValues();
	void calculateOtherLayersNodeValues(Layer& target, Layer& nextLayer);
	double nodeCost(double& value, double& expectedValue);
	double nodeCostDerivative(double& value, double& expectedValue);
	double totalCost();

	void fillInputVector(std::vector<double> inputVector);
	void fillExpectedVector(std::vector<double> expectedVector);

	void fillWeights();
	void fillWeights(double x);

	bool loadWeights(const char fileName[]);
	bool saveWeights(const char fileName[]);

	double activation(double x);
	double activationDerivative(double x);

};

