#ifndef ALGORITHME_GENETIQUE_H
#define ALGORITHME_GENETIQUE_H

#include <QVector>
#include <QPair>

#include "imppt.h"

class Algorithme_Genetique : public IMPPT
{
public:
    Algorithme_Genetique(int pop_size, int nb_iter, double max_val, double prob_crossver=0.75, double prob_mutation=0.1);
    ~Algorithme_Genetique();

    double searchMPP(double tension, double courant);

private:
    QVector<int> * best_members(int k);

private:
    int _taille_population;

    QVector<double> * _population;     // valeurs de tension
    QVector<double> * _fitness;        // valeurs de puissances

    int _nb_iter;
    double _max_val;
    double _prob_crossover;
    double _prob_mutation;

    QPair<double,double> _mpp;

    bool _init;
};

#endif // ALGORITHME_GENETIQUE_H
