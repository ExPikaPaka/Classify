#include "entMath.h"


namespace ent {
	namespace math {
		f64 mapValue(f64 value, f64 fromMin, f64 fromMax, f64 toMin, f64 toMax) {
            // Ensure the value is within the source range
            f64 clampedValue = std::max(fromMin, std::min(value, fromMax));

            // Calculate the mapped value in the target range
            f64 mappedValue = toMin + (clampedValue - fromMin) * ((toMax - toMin) / (fromMax - fromMin));

            return mappedValue;
        }

        f64 clampValue(f64 value, f64 min, f64 max) {
            return value < min ? min : (value > max ? max : value);
        }

        bool inRange(f64 value, f64 min, f64 max) {
            return value >= min && value <= max;
        }

        f64 lerp(f64 min, f64 max, f64 t) {
            return min + (max - min) * t;
        }
	}
}