#include "icmppttracker.h"

ICMpptTracker::ICMpptTracker(double pas):
    _pas(pas),
    _old_courant(0.0),
    _old_tension(0.0)
{

}

ICMpptTracker::~ICMpptTracker()
{

}

double ICMpptTracker::searchMPP(double tension, double courant)
{
    double vref;
    if (tension < 0 || courant < 0)
    {
        _old_tension = tension;
        _old_courant = courant;
        return 0.0;
    }

    double _ic = courant/tension;
    double _deltat_ic;

    double _delta_tension = tension-_old_tension;
    double _delta_courant = courant-_old_courant;


    if(_delta_tension == 0){
        if(_delta_courant == 0){
            vref = tension;
        }
        else if(_delta_courant > 0){
            vref = tension+_pas;
        }
        else{
            vref = tension -_pas;
        }
    }
    else{
        _deltat_ic = _delta_courant/_delta_tension;
        if(_deltat_ic > -_ic){
            vref = tension+_pas;
        }
        else if(_deltat_ic < -_ic){
            vref = tension -_pas;
        }
        else{
            vref=tension;
        }
    }
    _old_tension = tension;
    _old_courant = courant;
    return vref;
}
