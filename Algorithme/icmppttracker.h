#ifndef ICMPPTTRACKER_H
#define ICMPPTTRACKER_H

#include "imppt.h"
#include <QObject>

class ICMpptTracker : public IMPPT
{
public:
    ICMpptTracker(double pas);
    ~ICMpptTracker();

    double searchMPP(double tension, double courant);

private:
    double _pas;
    double _old_tension;
    double _old_courant;
};

#endif // ICMPPTTRACKER_H
