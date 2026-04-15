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
        double a = 0.5, b = 3.0;
        for(int i=0; i<DIMENSIONS; ++i) { 
            double sum1 = 0; 
            for(int k=0; k<=11; ++k) { 
                sum1 += pow(a,k) * cos(2.0*M_PI*pow(b,k)*(vector[i]+0.5)); 
            } 
            result += sum1; 
        }
        double sum2 = 0;
        for(int k=0; k<=11; ++k) { 
            sum2 += pow(a,k) * cos(2.0*M_PI*pow(b,k)*0.5); 
        }
        return result - DIMENSIONS * sum2;
    }
}
