#ifndef ICOURBE_H
#define ICOURBE_H

#include <QVector>

#define NB_VALUES 1000

struct _valuesIV {
    double tension;
    double courant;

public:
    _valuesIV()
        : tension(0.)
        , courant(.0)
    {
    }

    _valuesIV(double t,double c)
        : tension(t)
        , courant(c)
    {}
};

class ICourbe
{
public:
    virtual const QVector<_valuesIV>* getCourbeData() const=0;
    virtual const QPointF getCourbePoint() const = 0;
    virtual double getCourbeIsc() const = 0;
    virtual double getCourbeVoc() const = 0;
    virtual int getNpp() const = 0;
    virtual int getNss() const = 0;
};

#endif // ICOURBE_H
