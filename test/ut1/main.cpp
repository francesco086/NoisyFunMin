#include <cassert>
#include <iostream>

#include "nfm/1DTools.hpp"
#include "nfm/ConjGrad.hpp"
#include "nfm/LogManager.hpp"

#include "TestNFMFunctions.hpp"

nfm::NoisyBracket prepareBracket(nfm::NoisyFunction &fun, const double ax, const double cx)
{
    nfm::NoisyBracket bracket{{ax,            {}},
                              {0.5*(ax + cx), {}},
                              {cx,            {}}}; // bracket from ax to cx
    std::vector<double> xvec(1);
    xvec[0] = bracket.a.x;
    bracket.a.f = fun(xvec);
    xvec[0] = bracket.b.x;
    bracket.b.f = fun(xvec);
    xvec[0] = bracket.c.x;
    bracket.c.f = fun(xvec);
    return bracket;
}

void assertBracket(const nfm::NoisyBracket &bracket, const double maxax, const double mincx)
{
    const nfm::NoisyIOPair1D &a = bracket.a;
    const nfm::NoisyIOPair1D &b = bracket.b;
    const nfm::NoisyIOPair1D &c = bracket.c;

    assert(a.x < maxax);
    assert(c.x > mincx);
    assert(a.x != b.x);
    assert(c.x != b.x);
    assert(a.f > b.f);
    assert(c.f > b.f);
}

int main()
{
    using namespace std;
    using namespace nfm;

    // test some log manager stuff
    LogManager::setLoggingOn(true);
    assert(LogManager::isLoggingOn());
    assert(LogManager::isVerbose());
    LogManager::setLoggingOn(false);
    assert(LogManager::isLoggingOn());
    assert(!LogManager::isVerbose());
    LogManager::setLoggingOff();
    assert(!LogManager::isLoggingOn());
    assert(!LogManager::isVerbose());

    //LogManager::setLoggingOn(true); // uncomment if you actually want printout

    // a noisy function input/output pair and a bracket
    NoisyBracket bracket{};
    bool flag_success;

    // check parabola   x^2   ...
    Parabola parabola;

    // ... starting from interval [-1000, 1]
    bracket = prepareBracket(parabola, -1000., 1.);
    flag_success = nfm::findBracket(parabola, bracket);
    assert(flag_success);
    assertBracket(bracket, 0., 0.);

    // ... starting from interval [-5, 1000]
    LogManager::logString("\n\n=========================================================================\n\n");
    bracket = prepareBracket(parabola, 1000., -5.); // should be sorted in findBracket
    flag_success = nfm::findBracket(parabola, bracket);
    assert(flag_success);
    assertBracket(bracket, 0., 0.);

    // ... starting from interval [-1.5,10]
    LogManager::logString("\n\n=========================================================================\n\n");
    bracket = prepareBracket(parabola, -1.5, 10.);
    flag_success = nfm::findBracket(parabola, bracket);
    assert(flag_success);
    assertBracket(bracket, 0., 0.);


    // check well function   -1 if (-1 < x < 1) else +1   ...
    Well well;

    // ... starting from interval [-1.25, 3.5]
    LogManager::logString("\n\n=========================================================================\n\n");
    bracket = prepareBracket(well, -1.25, 5.5);
    flag_success = nfm::findBracket(well, bracket);
    assert(flag_success);
    assertBracket(bracket, -1., 1.);

    // ... starting from interval [-1000, 500]
    LogManager::logString("\n\n=========================================================================\n\n");
    bracket = prepareBracket(well, -1000., 100.);
    flag_success = nfm::findBracket(well, bracket);
    assert(!flag_success);

    // ... starting from interval [-1, 3]
    LogManager::logString("\n\n=========================================================================\n\n");
    bracket = prepareBracket(well, -1.1, 3.);
    flag_success = nfm::findBracket(well, bracket);
    assert(flag_success);
    assertBracket(bracket, -1., 1.);

    return 0;
}
