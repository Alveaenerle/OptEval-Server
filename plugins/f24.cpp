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
        double sum1 = 0.0, sum2 = 0.0, sum3 = 0.0;
        double mu0 = 2.5;
        double d = 1.0;
        double s = 1.0 - 0.5 / sqrt(DIMENSIONS + 20.0);
        for(int i=0; i<DIMENSIONS; ++i) { 
            double x = vector[i]; 
            sum1 += (x - mu0)*(x - mu0); 
            sum2 += (x + mu0)*(x + mu0); 
            sum3 += 1.0 - cos(2.0*M_PI*(x - mu0)); 
        }
        return std::min(sum1, d * DIMENSIONS + s * sum2) + 10.0 * sum3;
    }
}
