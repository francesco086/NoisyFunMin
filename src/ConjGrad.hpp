#ifndef CONJ_GRAD
#define CONJ_GRAD

#include "NoisyFunMin.hpp"
#include "NoisyFunction.hpp"

class ConjGrad: public NFM{

private:
    bool _use_conjgrad;

protected:
    // --- Internal methods
    void findNextX(const double * dir, double &deltatargetfun, double &deltax);
    void _writeCGDirectionInLog(const double * dir, const std::string &name);

public:
    ConjGrad(NoisyFunctionWithGradient * targetfun, const bool useGradientError = true, const size_t &max_n_const_values = 20): NFM(targetfun, useGradientError, max_n_const_values)
    {
        setGradientTargetFun(targetfun);
        _use_conjgrad = true;
    }
    ~ConjGrad(){ }


    // Configuration
    void configureToFollowSimpleGradient(){_use_conjgrad = false;};  // make ConjGrad a Steepest Descent

    // --- Minimization
    void findMin();
};


#endif
