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
        double result = vector[0]*vector[0];
        double sum = 0.0;
        for(int i=1; i<DIMENSIONS; ++i) sum += vector[i]*vector[i];
        result += 100.0 * sqrt(sum);
        return result;
    }
}
