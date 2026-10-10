#ifndef COLDCHAINML_H
#define COLDCHAINML_H

namespace Eloquent {
    namespace ML {
        namespace Port {
            class DecisionTree {
                public:
                    /**
                     * Predict spoilage status from sensor features:
                     * x[0] = temperature_c
                     * x[1] = humidity_pct
                     * x[2] = light_adc
                     * x[3] = vibration_m_s2
                     * 
                     * Returns: 0 = SAFE, 1 = WARNING, 2 = SPOILED
                     */
                    int predict(float *x) {
                        if (x[0] > 12.0 || x[2] > 1500.0) {
                            return 2; // Critical / Spoiled
                        }
                        if (x[0] > 6.5 || x[3] > 3.0) {
                            return 1; // Warning / Elevated Risk
                        }
                        return 0;     // Safe
                    }
            };
        }
    }
}

#endif