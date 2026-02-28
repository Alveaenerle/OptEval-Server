#include <cmath>

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
        double result = 0.0;
        for (int i = 0; i < DIMENSIONS; ++i) {
            result += vector[i] * vector[i];
        }
        return result;
    }

}