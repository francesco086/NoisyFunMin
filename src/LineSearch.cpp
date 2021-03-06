#include "nfm/LineSearch.hpp"
#include "nfm/LogManager.hpp"

#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>


// --- Internal Functions

// - Non-throwing checks

// check bracket for position tolerances:
// - epsx: Minimum bracket size / numerical tolerance
inline bool checkBracketXTol(const nfm::NoisyBracket &bracket, const double epsx)
{
    return (fabs(bracket.c.x - bracket.b.x) > epsx && fabs(bracket.b.x - bracket.a.x) > epsx); // simply check for the position distances
}

// check bracket for function value tolerances (noisy version)
// - epsf: Minimal noisy value distance between a<->b or b<->c
inline bool checkBracketFTol(const nfm::NoisyBracket &bracket, const double epsf)
{
    const bool checkLeft = bracket.a.f.minDist(bracket.b.f) > epsf;
    const bool checkRight = bracket.c.f.minDist(bracket.b.f) > epsf;
    return checkLeft && checkRight;
}

// are there neighbouring values that are equal (within error) ?
inline bool hasEquals(const nfm::NoisyBracket &bracket)
{
    return ((bracket.a.f == bracket.b.f) || (bracket.b.f == bracket.c.f));
}

// does bracket fulfill the bracketing condition a.f. > b.f < c.f
inline bool isBracketed(const nfm::NoisyBracket &bracket)
{
    return (bracket.a.f > bracket.b.f && bracket.b.f < bracket.c.f);
}

// - Throwing checks

// throw on invalid bracket X
inline void validateBracketX(const double ax, const double bx, const double cx, const std::string &callerName)
{
    if (ax >= cx) {
        throw std::invalid_argument("[" + callerName + "->validateBracketX] Bracket violates (a.x < c.x).");
    }
    if (bx >= cx || bx <= ax) {
        throw std::invalid_argument("[" + callerName + "->validateBracketX] Bracket violates (a.x < b.x < c.x).");
    }
}

// throw on invalid bracket
inline void validateBracket(const nfm::NoisyBracket &bracket, const std::string &callerName)
{
    validateBracketX(bracket.a.x, bracket.b.x, bracket.c.x, callerName);
    if (!isBracketed(bracket)) {
        throw std::invalid_argument("[" + callerName + "->validateBracket] Bracket violates (a.f > b.f < c.f).");
    }
}

// Other helper functions
template <class T>
inline void shiftABC(T &a, T &b, T &c, const T d)
{
    a = b;
    b = c;
    c = d;
}

inline nfm::NoisyBracket sortedBracket(nfm::NoisyBracket bracket) // we take value and return by value
{   // make sure bracket x's are in ascending order (assuming b is bracketed)
    if (bracket.a.x > bracket.c.x) { std::swap(bracket.a, bracket.c); }
    return bracket;
}

void writeBracketToLog(const std::string &key, const nfm::NoisyBracket &bracket)
{
    using namespace nfm;
    if (!LogManager::isLoggingOn()) { return; } // save time when loggin is off
    std::stringstream s;
    s << key << ":    " <<
      bracket.a.x << " -> " << bracket.a.f << "    " <<
      bracket.b.x << " -> " << bracket.b.f << "    " <<
      bracket.c.x << " -> " << bracket.c.f;
    s << std::flush;
    LogManager::logString(s.str(), LogLevel::VERBOSE);
}

// --- Public Functions

namespace nfm
{

bool findBracket(NoisyFunction &f1d, NoisyBracket &bracket /*inout*/, const int maxNIter, double epsx)
{
    // Noisy findBracket Algorithm
    // Mix of own ideas and GSL's findBracket ( gsl/min/bracketing.c )
    // Returns true when valid bracket found, else false.
    using namespace m1d_detail;

    // --- Sanity
    if (f1d.getNDim() != 1) {
        throw std::invalid_argument("[nfm::findBracket] The NoisyFunction is not 1D. Ndim=" + std::to_string(f1d.getNDim()));
    }
    bracket = sortedBracket(bracket); // ensure proper ordering
    validateBracketX(bracket.a.x, bracket.b.x, bracket.c.x, "nfm::findBracket"); // ensure valid bracket (else throw)
    epsx = std::max(0., epsx);

    // --- Initialization
    NoisyIOPair1D &a = bracket.a;
    NoisyIOPair1D &b = bracket.b;
    NoisyIOPair1D &c = bracket.c;

    // shortcut lambda
    int iter = 0; // keeps track of loop iteration count (overall)
    std::vector<double> xvec(1); // helper array to invoke noisy function
    std::function<NoisyValue(double x)> F = [&](const double x)
    {
        xvec[0] = x;
        return f1d(xvec);
    };

    // --- Bracketing

    // Pre-Processing
    writeBracketToLog("findBracket init", bracket);
    while (hasEquals(bracket)) { // we need larger interval
        // check stopping conditions
        if (!checkBracketXTol(bracket, epsx)) { return false; }
        if (isBracketed(bracket)) {
            writeBracketToLog("findBracket final", bracket);
            return true; // return with early success
        }
        if (iter++ > maxNIter) { return false; } // evaluation limit

        // scale up
        b = c;
        c.x = a.x + (b.x - a.x)/IGOLD2;
        c.f = F(c.x);

        writeBracketToLog("findBracket pre-step (scale)", bracket);
    }

    // Main Loop
    while (!hasEquals(bracket)) { // stop if we have equal function values again
        // check other stopping conditions
        if (!checkBracketXTol(bracket, epsx)) { return false; } // bracket violates tolerances
        if (isBracketed(bracket)) {
            writeBracketToLog("findBracket final", bracket);
            return true; // return with success (i.e. a.f > b.f < c.f ruled out below)
        }
        if (iter++ > maxNIter) { return false; } // evaluation limit

        // regular iteration (equals ruled out)
        if (b.f < a.f) { // -> a.f > b.f > c.f (else we would have returned successful)
            // move up
            shiftABC(a.x, b.x, c.x, (c.x - b.x)/IGOLD2 + b.x);
            shiftABC(a.f, b.f, c.f, F(c.x));
            writeBracketToLog("findBracket step (move)", bracket);
        }
        else { // -> a.f < b.f < c.f || a.f < b.f > c.f
            // contract
            c = b;
            b.x = (c.x - a.x)*IGOLD2 + a.x;
            b.f = F(b.x);
            writeBracketToLog("findBracket step (contract)", bracket);
        }
    }
    return false;
}

NoisyIOPair1D brentMin(NoisyFunction &f1d, NoisyBracket bracket, const int maxNIter, double epsx, double epsf)
{
    //
    // Adaption of GNU Scientific Libraries's Brent minimization ( gsl/min/brent.c )
    //
    using namespace m1d_detail;

    // Sanity
    if (f1d.getNDim() != 1) {
        throw std::invalid_argument("[nfm::brentMin] The NoisyFunction is not 1D. Ndim=" + std::to_string(f1d.getNDim()));
    }
    validateBracket(bracket, "nfm::brentMin"); // check for valid bracket
    epsx = std::max(0., epsx);
    epsf = std::max(0., epsf);

    // --- Initialization

    // we reuse the bracket
    NoisyIOPair1D &lb = bracket.a; // lower bound
    NoisyIOPair1D &m = bracket.b;
    NoisyIOPair1D &ub = bracket.c; // upper bound

    // shortcut lambda
    std::vector<double> xvec(1); // helper array to invoke noisy function
    std::function<NoisyValue(double x)> F = [&](const double x)
    {
        xvec[0] = x;
        return f1d(xvec);
    };

    // initialize helpers
    double d = 0., e = 0.;
    NoisyIOPair1D v{}, w{};
    v.x = lb.x + IGOLD2*(ub.x - lb.x);
    v.f = F(v.x);
    w = v;

    // --- Main Brent Loop
    for (int it = 0; it < maxNIter; ++it) {
        if (!checkBracketXTol(bracket, epsx)) { break; } // bracket size too small, return early
        if (!checkBracketFTol(bracket, epsf)) { break; } // values too close, return early (noisy version)

        const double mtolb = m.x - lb.x;
        const double mtoub = ub.x - m.x;
        const double xm = 0.5*(lb.x + ub.x);
        const double tol = 1.5e-08*fabs(m.x); // tolerance for strategy choice

        //std::swap(d, e); // this happens only in the step initialization of GSL's Brent and looks like a bug
        NoisyIOPair1D u{};

        double p = 0.;
        double q = 0.;
        double r = 0.;

        bool flag_parab; // was parabolic fit used

        // The following is a customized adaption of GSL's Brent,
        // just using NoisyValue overloads and small other modifications.
        if (fabs(e) > tol) { // we should fit a parabola
            r = (m.x - w.x)*(m.f.val - v.f.val);
            q = (m.x - v.x)*(m.f.val - w.f.val);
            p = (m.x - v.x)*q - (m.x - w.x)*r;
            q = 2.*(q - r);

            if (q > 0.) {
                p = -p;
            }
            else {
                q = -q;
            }
            r = e;
            e = d;
        }

        // if parabola fine, use it
        if (fabs(p) < fabs(0.5*q*r) && p < q*mtolb && p < q*mtoub) {
            double t2 = 2.*tol;
            d = p/q;
            u.x = m.x + d;
            if ((u.x - lb.x) < t2 || (ub.x - u.x) < t2) { // keep minimal distance to lb and ub
                d = (m.x < xm) ? tol : -tol;
            }
            flag_parab = true;
        }
        else { // else use golden section
            e = (m.x < xm) ? ub.x - m.x : -(m.x - lb.x);
            d = IGOLD2*e;
            flag_parab = false;
        }

        // keep minimal distance to m
        if (fabs(d) >= tol) {
            u.x = m.x + d;
        }
        else {
            u.x = m.x + ((d > 0) ? tol : -tol);
        }

        // here we evaluate the function
        u.f = F(u.x);

        // check continue conditions
        if (u.f.getUBound() <= m.f.getUBound()) { // keep best ubound in m (prove safer so far)
            if (u.x < m.x) { ub = m; }
            else { lb = m; }

            v = w;
            w = m;
            m = u;
        }
        else {
            if (u.x < m.x) { lb = u; }
            else { ub = u; }

            if (u.f <= w.f || w.x == m.x) {
                v = w;
                w = u;
            }
            else if (u.f <= v.f || v.x == m.x || v.x == w.x) {
                v = u;
            }
        }

        writeBracketToLog(flag_parab ? "brentMin step (parabola)" : "brentMin step (goldsect)", bracket);
    }

    writeBracketToLog("brentMin final", bracket);

    // Return point in m. v might rarely have a better upper bound, but is more risky.
    // To avoid any bias, we recompute the function value at the final position.
    m.f = F(m.x);
    return m;
}


NoisyIOPair multiLineMin(NoisyFunction &mdf, NoisyIOPair p0Pair, const std::vector<double> &dir, MLMParams params)
{
    using namespace m1d_detail;
    // Sanity
    if (mdf.getNDim() != p0Pair.getNDim() || p0Pair.x.size() != dir.size()) {
        throw std::invalid_argument("[nfm::multiLineMin] The passed function and positions are inconsistent in size.");
    }
    if (params.stepLeft < 0. || params.stepRight <= 0.) {
        throw std::invalid_argument("[nfm::multiLineMin] stepLeft and stepRight must be non-negative (stepRight strictly positive).");
    }
    // these should be non-zero
    params.epsx = (params.epsx > 0) ? params.epsx : STD_XTOL;
    params.epsf = (params.epsf > 0) ? params.epsf : STD_FTOL;

    // project the original multi-dim function into a one-dim function
    FunProjection1D proj1d(&mdf, p0Pair.x, dir);

    // prepare initial bracket (allow backstep via stepLeft)
    const double ax = -params.stepLeft;
    const double cx = params.stepRight;
    const double bx = ax + (cx - ax)*IGOLD2; // golden section
    NoisyBracket bracket{{ax, (fabs(ax) == 0.) ? p0Pair.f : proj1d(ax)}, // avoid recomputation
                         {bx, proj1d(bx)},
                         {cx, proj1d(cx)}};

    if (findBracket(proj1d, bracket, params.maxNBracket, params.epsx)) { // valid bracket was stored in bracket
        // now do line-minimization via brent
        NoisyIOPair1D min1D = brentMin(proj1d, bracket, params.maxNMinimize, params.epsx, params.epsf);

        if (min1D.f <= p0Pair.f) { // reject new values that are truly larger
            // return new NoisyIOPair
            p0Pair.f = min1D.f; // store the minimal f value
            proj1d.getVecFromX(min1D.x, p0Pair.x); // get the true x position
            return p0Pair;
        }
    }
    // return the old position, but recompute value
    p0Pair.f = mdf(p0Pair.x);
    return p0Pair;
}
} // namespace nfm
