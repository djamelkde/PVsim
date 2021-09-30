#ifndef PERTURB_AND_OBSERVE_H
#define PERTURB_AND_OBSERVE_H

#include <QThread>

#include "imppt.h"

class Perturb_and_Observe : public IMPPT
{
public:
    Perturb_and_Observe(double pas);

    double searchMPP(double tension, double courant);

private:
    double _puissance;
    double _pas;
};

#endif // PERTURB_AND_OBSERVE_H
