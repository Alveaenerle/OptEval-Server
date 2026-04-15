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
        double result = 1.0;
        for(int i=0; i<DIMENSIONS; ++i) { 
            double sum = 0.0; 
            for(int j=1; j<=32; ++j) { 
                sum += std::abs(pow(2.0, j)*vector[i] - floor(pow(2.0, j)*vector[i] + 0.5)) / pow(2.0, j); 
            } 
            result *= pow(1.0 + (i+1)*sum, 10.0/pow((double)DIMENSIONS, 1.2)); 
        }
        return 10.0 / DIMENSIONS / DIMENSIONS * result - 10.0 / DIMENSIONS / DIMENSIONS;
    }
}
