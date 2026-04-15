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
        double result = 0.0;
        for(int i=0; i<DIMENSIONS-1; ++i) { 
            double t1 = vector[i]*vector[i] - vector[i+1]; 
            double t2 = vector[i] - 1.0; 
            result += 100.0 * t1*t1 + t2*t2; 
        }
        return result;
    }
}
