                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               #ifndef ACOMPPTTRACKER_H
#define ACOMPPTTRACKER_H

#include <QObject>
#include <QVector>
#include "imppt.h"

#define DELTA 0.04


struct _valueWithFitness {
    double tension;
    double fitness;

public:
    _valueWithFitness()
        : tension(0.)
        , fitness(.0)
    {
    }

    _valueWithFitness(double t,double c)
        : tension(t)
        , fitness(c)
    {}
};

class AcoMPPTTracker : public IMPPT
{
public:
    AcoMPPTTracker(int nbAnt=5, int archiveLength=8, int nbIter=20, double _maxValue = 108.0, double alpha=3.5, double sigma=0.005, double _probaEvaporation=0.3);
    ~AcoMPPTTracker();
    double searchMPP(double tension, double courant);
    double generateValueAnt();
    void sortArchive();
    void keepBestSolutions();
private:
    int nbAnt;
    int archiveLength;
    double _maxValue;
    double alpha;
    double sigma;
    double _probaEvaporation;
    int nbIter;
    QVector<_valueWithFitness>* archive;
    QVector<_valueWithFitness>* generatedSolutions;
    QVector<double>* pheromone;
    bool _init;
    double Pthr;
    double tension_opt;
    double P_old;
    bool _resetArchive;
    double valueInitialeOfAlpha;

};

#endif // ACOMPPTTRACKER_H
