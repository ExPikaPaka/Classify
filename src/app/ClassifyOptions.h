#pragma once

// Classify method type
enum class ClassifyMethod {
	minMax,
	averageDist,
	angle,
	perceptron,
	genetic,
}; 

enum class MaskType {
	equal,
	eqOrLower,
	eqOrHigher
};

enum class SectorType {
	rectangular,
	circle
};

enum class VectorNormalization {
	normal,
	mod
};