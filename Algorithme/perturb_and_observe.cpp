#include "perturb_and_observe.h"

Perturb_and_Observe::Perturb_and_Observe(double pas)
    : _puissance(.0)
    , _pas(pas)
{
}

double Perturb_and_Observe::searchMPP(double tension, double courant)
{
    if (tension < 0 || courant < 0)
    {
        return .0;
    }

    double p = tension * courant;
//qDebug("V : %e, I : %e", tension, courant);
    if (p < _puissance)
    {
        _pas *= -1.;
    }

    _puissance = p;
    return tension + _pas;
}
