#include "algorithme_genetique.h"

Algorithme_Genetique::Algorithme_Genetique(int pop_size, int nb_iter, double max_val, double prob_crossover, double prob_mutation)
    : _taille_population(pop_size)
    , _population(new QVector<double>(pop_size))
    , _fitness(new QVector<double>(pop_size))
    , _nb_iter(nb_iter)
    , _max_val(max_val)
    , _prob_crossover(prob_crossover)
    , _prob_mutation(prob_mutation)
    , _mpp(QPair<double,double>(-1.0,-1.0))
    , _init(false)
{

}

Algorithme_Genetique::~Algorithme_Genetique()
{
    delete _population;
    delete _fitness;
}

double Algorithme_Genetique::searchMPP(double tension, double courant)
{
    if (tension < 0 || courant < 0)
    {
        return _max_val * (((double)rand()) / RAND_MAX);
    }

    static int init_index = 0;
    static bool init = false;
    static int iter_index = 0;

    if(!_init){
        init_index=0;
        iter_index = 0;
        init = false;
        _init=true;
    }

    if (init_index < _taille_population)
    {
        double puissance = tension * courant;
        (*_population)[init_index] = tension;
        (*_fitness)[init_index] = puissance;
        if (puissance > _mpp.second)
        {
            _mpp.first = tension;
            _mpp.second = puissance;
        }
        ++init_index;
        if (init_index >= _taille_population)
        {
            init = true;
        }
        else
        {
            if (!init)
            {
                return _max_val * (((double)rand()) / RAND_MAX);
            }
            else
            {
                return _population->at(init_index);
            }
        }
    }


    if (_nb_iter <= 0 || iter_index < _nb_iter)
    {
        // selection
        int nb_select = _taille_population / 2;
        /*
            sort population
            create list of best solutions indexes
         */
        QVector<int> * best = best_members(nb_select);
        // crossover
        /*
            crossover the best solutions
         */
        QVector<double> * new_popu = new QVector<double>();
        // for (int i = 0; i <= _taille_population/2; ++i)
        while (new_popu->size() < _taille_population)
        {
            int p1_index = best->at(rand() % nb_select);
            double p1 = _population->at(p1_index);
            int p2_index = best->at(rand() % nb_select);
            double p2 = _population->at(p2_index);
            double f1 = p1;
            double f2 = p2;

            // crossover
            double cross = ((double)rand()) / RAND_MAX;
            if (cross < _prob_crossover)
            {
                double alpha = ((double)rand()) / RAND_MAX;
                f1 = alpha*p1 + (1-alpha)*p2;
                f2 = (1-alpha)*p1 + alpha*p2;
            }
            // mutation
            double mutate = ((double)rand()) / RAND_MAX;
            if (mutate < _prob_mutation)
            {
                double max_noise = _max_val/2;
                double tmp1 = ((double)rand())/RAND_MAX * (2*max_noise) - max_noise;
                double tmp2 = ((double)rand())/RAND_MAX * (2*max_noise) - max_noise;
                double f1_noise = (tmp1 + tmp2) / 2;
                f1 += f1_noise;
                f1 = f1 < 0 ? 0 : f1 > _max_val ? _max_val : f1;
                tmp1 = ((double)rand())/RAND_MAX * (2*max_noise) - max_noise;
                tmp2 = ((double)rand())/RAND_MAX * (2*max_noise) - max_noise;
                double f2_noise = (tmp1 + tmp2) / 2;
                f2 += f2_noise;
                f2 = f2 < 0 ? 0 : f2 > _max_val ? _max_val : f2;
            }
            //
            new_popu->append(f1);
            new_popu->append(f2);
        }
        new_popu->resize(_taille_population);
        // update population
        delete best;
        delete _population;
        _population = new_popu;
        init_index = 0;
        // next iteration
        ++iter_index;
        return _population->at(0);
    }

    return _mpp.first;
}


QVector<int> * Algorithme_Genetique::best_members(int k)
{
    QVector<int> * sorted = new QVector<int>();

    for (int i(0); i < _taille_population; ++i)
    {
        int j = 0;
            
        for (; j < sorted->size(); ++j)
        {
            if (_fitness->at(sorted->at(j)) < _fitness->at(i))
            {
                sorted->insert(j, i);
                break;
            }
        }

        if (j >= sorted->size())
        {
            sorted->append(i);
        }
    }
    sorted->resize(k);
    
    return sorted;
}
