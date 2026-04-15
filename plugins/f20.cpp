#include <cmath>
#include <algorithm>

const int DIMENSIONS = 10;
const double MIN_BOUND = -5.0;
const double MAX_BOUND = 5.0;

extern "C" {
    int get_dimensions() {
        return DIMENSIONS;
    }

    bool check_constraints(const double* vector) {
        for (int i = 0; i < DIMENSIONS; ++i) {
            if (vector[i] < MIN_BOUND || vector[i] > MAX_BOUND) {
                return false;
            }
        }
        return true;
    }

    double evaluate(const double* vector) {
        double result = 418.9828872724338 * DIMENSIONS;
        for(int i=0; i<DIMENSIONS; ++i) { 
            double z = vector[i] * 100.0; // scale up loosely to original Schwefel bounds
            result -= z * sin(sqrt(std::abs(z))); 
        }
        return result;
    }
}
