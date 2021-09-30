#ifndef PSOMPPTTRACKER_H
#define PSOMPPTTRACKER_H
#include <QObject>
#include <QVector>
#include "imppt.h"


class PSOMpptTracker : public IMPPT
{
public:
    PSOMpptTracker(int nbParticules = 4, int nbIter = 20, double _maxValue =168.0, double _c1 = 1.2, double _c2 = 1.6, double _w = 0.4);
    ~PSOMpptTracker();
    double searchMPP(double tension, double courant);
    void initialisation();
private:
    int nbParticules;
    int nbIter;
    double _maxValue;
    double _c1;
    double _c2;
    double _w;
    double Gbest;
    QVector<double>* Pbest;
    QVector<double>* velocities;
    QVector<double>* partculePositions;
    QVector<double>* fitnessOfBestParticules;
    double fitnessGbest;
    bool _init;

    double P_old;
    double K1;
    double K2;
    double Pthr;
};

#endif // PSOMPPTTRACKER_H
