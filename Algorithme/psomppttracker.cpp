#include "psomppttracker.h"
#include <QVector>
#include "imppt.h"

PSOMpptTracker::PSOMpptTracker(int _nbParticules, int _nbIter, double maxValue, double c1, double c2, double w)
    : nbParticules(_nbParticules)
    , nbIter(_nbIter)
    , _maxValue(maxValue)
    , _c1(c1)
    , _c2(c2)
    , _w(w)
    , Pbest(new QVector<double>(_nbParticules))
    , velocities(new QVector<double>(_nbParticules))
    , partculePositions(new QVector<double>(_nbParticules))
    , fitnessOfBestParticules(new QVector<double>(_nbParticules))
    , Gbest(0.0)
    , fitnessGbest(0.0)
    , _init(false)
    , K1(0.05*maxValue)
    , K2(0.015*maxValue)
    , Pthr(0.015)
{

}

PSOMpptTracker::~PSOMpptTracker()
{

}

double PSOMpptTracker::searchMPP(double tension, double courant)
{
    static int index_particule = 0;
    static int iter = 0;
    static int index_init_particule = 0;
    static int index_d;

    // initialisation phase;
    if(!_init){
        index_particule = 0;
        index_init_particule = 0;
        iter=0;
        index_d =0;
        initialisation();
        _init=true;
    }
    double fitness_particule;
    if(index_init_particule < nbParticules){
        if(index_init_particule > 0){
            fitness_particule = tension*courant;
            if(fitness_particule > fitnessGbest){
                Gbest = (*Pbest)[index_init_particule-1];
                fitnessGbest = fitness_particule;
            }
        }
        (*partculePositions)[index_init_particule] = _maxValue*(((double)rand()) / RAND_MAX);
        (*Pbest)[index_init_particule] = (*partculePositions)[index_init_particule];
        (*fitnessOfBestParticules)[index_init_particule]=fitness_particule;
        index_init_particule ++;
        return (*partculePositions)[index_init_particule-1];
    }
    // fin d'initialisation.
    else{
        double r1,r2;
        if(iter < nbIter){
            //initialise the value of the last particule.
            if(iter==0 && index_particule==0){
                fitness_particule = tension*courant;
                (*fitnessOfBestParticules)[nbParticules-1]=fitness_particule;
                if(fitness_particule > fitnessGbest){
                    Gbest = (*Pbest)[nbParticules-1];
                    fitnessGbest = fitness_particule;
                }
            }
            ///////////////////////////////////////////////////
            // start iterations.
            r1 = ((double)rand()) / RAND_MAX;
            r2 = ((double)rand()) / RAND_MAX;
            if(index_particule < nbParticules){
                if(index_particule > 0){
                    fitness_particule = tension*courant;
                    if(fitness_particule > (*fitnessOfBestParticules)[index_particule-1]){
                        (*Pbest)[index_particule-1] = (*partculePositions)[index_particule-1];
                        (*fitnessOfBestParticules)[index_particule-1] =fitness_particule;
                        if (fitness_particule > fitnessGbest){
                            Gbest = (*Pbest)[index_particule-1];
                            fitnessGbest = fitness_particule;
                        }
                    }
                }
                (*velocities)[index_particule] = _w*(*velocities)[index_particule] +
                                                  _c1*r1*((*Pbest)[index_particule]-(*partculePositions)[index_particule])+
                                                  _c2*r2*(Gbest-(*partculePositions)[index_particule]);
                (*partculePositions)[index_particule] = (*partculePositions)[index_particule] + (*velocities)[index_particule];
                index_particule ++;
                return (*partculePositions)[index_particule-1];
            }
            else{   // index_particule >= nbParticules
                // updating the value of the last particule.
                fitness_particule = tension*courant;
                if(fitness_particule > (*fitnessOfBestParticules)[index_particule-1]){
                    (*Pbest)[nbParticules-1] = (*partculePositions)[nbParticules-1];
                    (*fitnessOfBestParticules)[index_particule-1] = fitness_particule;
                    if (fitness_particule > fitnessGbest){
                        Gbest = (*Pbest)[nbParticules-1];
                        fitnessGbest = fitness_particule;
                    }
                }
                iter ++;
                index_particule=0;
                P_old = fitnessGbest; // the old value of fitnessGbest.
                return Gbest;   // return the best particule in th swarm.
            }
        }
        else{ // iter > nbIter.
            double P_new = tension*courant;
            double deltaP = P_new-P_old;
            P_old = P_new;
            double rnd = ((double)rand()) / RAND_MAX;
            if(deltaP > Pthr*_maxValue || deltaP < -Pthr*_maxValue){
                if(rnd < 0.33){
                    (*partculePositions)[index_d] = Gbest - deltaP/K1;
                }
                else if(rnd < 0.66){
                    (*partculePositions)[index_d] = Gbest + ((deltaP>0)?K2:K2/2);
                }
                else{
                    (*partculePositions)[index_d] = Gbest - ((deltaP>0)?K2:K2/2);
                }
                index_d = (index_d+1)%nbParticules;
                return (*partculePositions)[(index_d-1)%nbParticules];
            }
            else{
                return Gbest;
            }
        }
    }
    return Gbest;
}

void PSOMpptTracker::initialisation()
{
    for(int i=0;i<nbParticules;i++){
        (*Pbest)[i] = 0.0;
        (*velocities)[i] = 0.0;
        (*partculePositions)[i] = 0.0;
        (*fitnessOfBestParticules)[i]=0.0;
        Gbest = 0.0;
    }
}


