#include "Adam.hpp"

#include "LogNFM.hpp"

#include <sstream>
#include <cmath>
#include <list>



// --- Log

void Adam::writeCurrentXInLog(){
    using namespace std;

    NFMLogManager log_manager = NFMLogManager();

    stringstream s;
    s << endl << "x:\n";
    for (int i=0; i<_x->getNDim(); ++i){
        s << _x->getX(i) << "    ";
    }
    s << endl << "    ->    value = " << _x->getF() << " +- " << _x->getDf() << endl;
    s << flush;
    log_manager.writeOnLog(s.str());
}


void Adam::writeDirectionInLog(const double * grad){
    using namespace std;

    NFMLogManager log_manager = NFMLogManager();

    stringstream s;
    s << endl << "direction to follow (and error):\n";
    for (int i=0; i<_x->getNDim(); ++i){
        s << -grad[i];
    }
    s << endl;
    s << flush;
    log_manager.writeOnLog(s.str());
}


void Adam::reportMeaninglessGradientInLog(){
    using namespace std;

    NFMLogManager log_manager = NFMLogManager();

    stringstream s;
    s << endl << "gradient seems to be meaningless, i.e. its error is too large" << endl;
    s << flush;
    log_manager.writeOnLog(s.str());
}


// --- Minimization

void Adam::findMin(){

    //initialize the gradient & moments
    double grad[_ndim], graderr[_ndim]; // gradient and (unused) error
    double m[_ndim], v[_ndim]; // moment vectors

    for (int i=0; i<_ndim; ++i) { // set all to 0
        grad[i] = 0.;
        graderr[i] = 0.;
        m[i] = 0.;
        v[i] = 0.;
    }

    const double afac = _alpha * sqrt(1-_beta2)/(1-_beta1); // helping factor

    NFMLogManager * log_manager = new NFMLogManager();
    log_manager->writeOnLog("\nBegin Adam::findMin() procedure\n");

    // compute the current value
    double newf, newdf;
    this->_gradtargetfun->f(_x->getX(), newf, newdf);
    _x->setF(newf, newdf);
    this->writeCurrentXInLog();

    //begin the minimization loop
    int step = 0;
    while ( isNotConverged() )
        {
            // compute the gradient
            this->_gradtargetfun->grad(_x->getX(), grad, graderr);
            this->writeDirectionInLog(grad);

            // compute the update
            for (int i=0; i<_ndim; ++i) {
                m[i] = _beta1 * m[i] + (1.-_beta1) * grad[i]; // Update biased first moment
                v[i] = _beta2 * v[i] + (1.-_beta2) * grad[i]*grad[i]; // Update biased second raw moment

                _x->setX(i, _x->getX(i) - afac * m[i] / (sqrt(v[i]) + _epsilon) ); // update _x
            }

            // compute the new value of the target function
            double newf, newdf;
            _gradtargetfun->f(_x->getX(), newf, newdf);
            _x->setF(newf, newdf);

            this->writeCurrentXInLog();

            step++;
        }

    log_manager->writeOnLog("\nEnd Adam::findMin() procedure\n");

    //free memory
    delete log_manager;
}


// --- Internal methods


bool Adam::isNotConverged(){
    using namespace std;

    NoisyFunctionValue * v = new NoisyFunctionValue(_x->getNDim());
    *v = *_x;
    _old_values.push_front(v);

    if (_old_values.size() > N_CONSTANT_VALUES_CONDITION_FOR_STOP){
        delete _old_values.back();
        _old_values.pop_back();
    }

    if (_old_values.size() == N_CONSTANT_VALUES_CONDITION_FOR_STOP){

        for (list<NoisyFunctionValue *>::iterator it = _old_values.begin(); it != _old_values.end(); ++it){
            if (it != _old_values.begin()){
                if (! (**it == **_old_values.begin()) ){
                    return true;
                }
            }
        }
        if (_old_values.size() < N_CONSTANT_VALUES_CONDITION_FOR_STOP)
            return true;

        NFMLogManager * log_manager = new NFMLogManager();
        log_manager->writeOnLog("\nCost function has stabilised, interrupting minimisation procedure.\n");
        delete log_manager;

        return false;
    }

    return true;
}
