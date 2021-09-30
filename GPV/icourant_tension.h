#ifndef ICOURANT_TENSION_H
#define ICOURANT_TENSION_H


class ICourant_Tension
{
public:
    virtual double getControlleurCourant() const = 0;
    virtual double getControlleurTension() const = 0;
    virtual double getControlleurIsc() const = 0;
    virtual double getControlleurVoc() const = 0;
    virtual void forcerCourantGPV(double) = 0;
    virtual void forcerTensionGPV(double) = 0;
};

#endif // ICOURANT_TENSION_H
