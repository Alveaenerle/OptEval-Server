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
        double result = 10.0 * DIMENSIONS;
        for(int i=0; i<DIMENSIONS; ++i) { 
            double s = (i%2==0 && vector[i]>0) ? 10.0 : 1.0; 
            double z = s * vector[i]; 
            result += z*z - 10.0 * cos(2.0 * M_PI * z); 
        }
        return result;
    }
}
