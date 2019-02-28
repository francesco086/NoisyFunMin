#ifndef NFM_ADAM_HPP
#define NFM_ADAM_HPP

#include "nfm/NoisyFunMin.hpp"
#include "nfm/NoisyFunction.hpp"
#include "nfm/NoisyFunctionValue.hpp"

#include <list>

class Adam: public NFM
{
protected:
    const bool _useAveraging; // use automatic exponential decaying (beta2) parameter averaging, as proposed in the end of Adam paper
    const double _alpha; // stepsize, default 0.001
    const double _beta1, _beta2; // decay rates in [0, 1), default 0.9 and 0.999 respectively
    const double _epsilon; // offset to stabilize division in update, default 10e-8

public:
    explicit Adam(NoisyFunctionWithGradient * targetfun, const bool useGradientError = false, const size_t &max_n_const_values = 20, const bool useAveraging = false, const double &alpha = 0.001, const double &beta1 = 0.9, const double &beta2 = 0.999, const double &epsilon = 10e-8)
        : NFM(targetfun, useGradientError, max_n_const_values), _useAveraging(useAveraging), _alpha(alpha), _beta1(beta1), _beta2(beta2), _epsilon(epsilon) { setGradientTargetFun(targetfun); }

    // --- Minimization
    void findMin() override;
};


#endif
