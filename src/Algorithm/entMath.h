#pragma once
#include "../dataTypes.h"
#include <algorithm>

namespace ent {
	namespace math {
		f64 mapValue(f64 value, f64 fromMin, f64 fromMax, f64 toMin, f64 toMax);

		f64 clampValue(f64 value, f64 min, f64 max);

		bool inRange(f64 value, f64 min, f64 max);

		f64 lerp(f64 min, f64 max, f64 t);
	}
}

