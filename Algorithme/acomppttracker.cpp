#include "acomppttracker.h"

AcoMPPTTracker::AcoMPPTTracker(int nbAnt, int archiveLength,
                               int nbIter, double _maxValue,
                               double alpha, double sigma,
                               double _probaEvaporation)
    : nbAnt(nbAnt)
    , archiveLength(archiveLength)
    , nbIter(nbIter)
    , _maxValue(_maxValue)
    , alpha(alpha)
    , valueInitialeOfAlpha(alpha)
    , sigma(sigma)
    , _probaEvaporation( _probaEvaporation)
    , archive(new QVector<_valueWithFitness>(archiveLength))
    , generatedSolutions(new QVector<_valueWithFitness>(nbAnt))
    , pheromone(new QVector<double>(nbAnt))
    , _init(false)
    , _resetArchive(false)
    , Pthr(0.015)
    , tension_opt(0.0)

{

}

AcoMPPTTracker::~AcoMPPTTracker()
{
    delete archive;
    delete generatedSolutions;
    delete pheromone;
}

double AcoMPPTTracker::searchMPP(double tension, double courant)
{
    // initialisation de l'archive.
    static int index_archive = 0;
    static int iter = 0;
    static int iterAnt = 0;
    static int index_reinitialisation;
    static int nb;

    if(!_init){
        index_archive=0;
        iter=0;
        iterAnt=0;
        index_reinitialisation = 0;
        _init=true;
        nb = 0;
    }
    if(index_archive < archive->size()){
        if(index_archive > 0){
            (*archive)[index_archive-1].fitness = tension*courant;
        }
        (*archive)[index_archive].tension = _maxValue*(((double)rand()) / RAND_MAX);
        index_archive++;
        return (*archive)[index_archive-1].tension;
    }
    // fin d'initialisation de l'archive.
    else{
        if(index_archive == archive->size()){
            (*archive)[archive->size()-1].fitness = courant*tension;
            index_archive++;
        }
        if(iter < nbIter){  // iter < nbIter
            if(iterAnt == 0){
                sortArchive();
                (*generatedSolutions)[0].tension = generateValueAnt();
                iterAnt++;
                return (*generatedSolutions)[0].tension;
            }
            else if(iterAnt < nbAnt){ // not the first ant.
                (*generatedSolutions)[iterAnt-1].fitness = courant*tension;
                (*generatedSolutions)[iterAnt].tension = generateValueAnt();
                iterAnt++;
                return (*generatedSolutions)[iterAnt-1].tension;
            }
            else{   // iterAnt > nbAnt.
                (*generatedSolutions)[nbAnt-1].fitness = courant*tension;
                keepBestSolutions();

                iter ++;// prochaine itération
                iterAnt = 0; // reinitialiser l'index à la première fourmi

                // critère d'évaporation
                alpha = (1 -_probaEvaporation)*alpha;
                tension_opt= (*archive)[0].tension;
                P_old = (*archive)[0].fitness;
                return (*archive)[0].tension;
            }
        }
        else{ // iter >= nbIter.
            double P_new = courant*tension;
            double deltaP = P_new-P_old;
            if(!_resetArchive){
                if(deltaP > Pthr*_maxValue || deltaP < -Pthr*_maxValue ){ // changement climatique detecté.
                    qDebug("anciens valeurs de l'archive------------------------");
                    for(int i=0;i<archiveLength;i++){
                        qDebug("V[%d]=%f, P[%d] = %f",i,(*archive)[0].tension,i,(*archive)[0].fitness);
                    }
                    qDebug("----------------------------------------------------");
                    qDebug("iter =(%d), P_new = (%f), P_old = (%f)",iter, P_new,P_old);
                    _resetArchive = true;
                }
            }
            if(_resetArchive){
                // reinitialisation de l'archive.
                if(index_reinitialisation > 0){
                    (*archive)[index_reinitialisation-1].fitness = tension*courant;
                }
                if(index_reinitialisation < archiveLength-1){
                    double rnd = ((double)rand()) / RAND_MAX;
                    double rd = ((double)rand()) / RAND_MAX;
                    double temp = (*archive)[index_reinitialisation].tension +
                                            rnd*((rd<0.50)? DELTA*_maxValue:-DELTA*_maxValue);
                    (*archive)[index_reinitialisation].tension= (temp<_maxValue)?temp:_maxValue;
                    index_reinitialisation = (index_reinitialisation+1)%archiveLength;
                    return (*archive)[index_reinitialisation-1].tension;
                }
                else{ // the last value of the archive.
                    iter = 0; // go to the first iteration.
                    iterAnt=0;
                    alpha = valueInitialeOfAlpha;
                    double rnd = ((double)rand()) / RAND_MAX;
                    double rd = ((double)rand()) / RAND_MAX;
                    double temp = (*archive)[index_reinitialisation].tension +
                                            rnd*((rd<0.50)? DELTA*_maxValue:-DELTA*_maxValue);
                    (*archive)[index_reinitialisation].tension= (temp<_maxValue)?temp:_maxValue ;
                    index_reinitialisation = 0;
                    _resetArchive = false;
                    for(int i=0; i <archiveLength ; i++){
                        qDebug("archive[%d]=( %f, %f)",i,(*archive)[i].tension,(*archive)[i].fitness);
                    }
                    qDebug(".......................");
                    return (*archive)[archiveLength-1].tension;
                }
            }
            else{
                return tension_opt;
            }
        }

    }
}

void AcoMPPTTracker::sortArchive()
{
    for(int i=0; i<archive->size()-1; i++)
    {
        for (int j=i+1;j<archive->size();j++)
        {
            if ((*archive)[i].fitness < (*archive)[j].fitness)
            {
                _valueWithFitness  temp = (*archive)[j];
                (*archive)[j]= (*archive)[i];
                (*archive)[i] = temp;
            }
        }
    }
}

double AcoMPPTTracker::generateValueAnt()
{
    double sum = 0.0;
    for(int k=0; k< nbAnt; k++){
        double ecartT =((*archive)[0].tension- (*archive)[k].tension);
        (*pheromone)[k] = exp(-ecartT*ecartT/(2*sigma));
        sum += (*pheromone)[k];
    }
    double x=((double)rand()) / RAND_MAX;

    // selectionner le point de référence.
    double cumul = (*pheromone)[0];
    int refPoint=0;

    while(x>=cumul/sum){
        refPoint++;
        cumul += (*pheromone)[refPoint];
    }

    double dx = 2*(((double)rand()) / RAND_MAX)*alpha - alpha;
    return (*archive)[refPoint].tension+dx;
}

void AcoMPPTTracker::keepBestSolutions()
{
    // sort ant's values.
    for(int i=0; i< generatedSolutions->size()-1; i++)
    {
        for (int j=i+1; j<generatedSolutions->size();j++)
        {
            if ((*generatedSolutions)[i].fitness < (*generatedSolutions)[j].fitness)
            {
                _valueWithFitness  temp = (*generatedSolutions)[j];
                (*generatedSolutions)[j]= (*generatedSolutions)[i];
                (*generatedSolutions)[i] = temp;
            }
        }
    }

    int i1 = 0;
    int i2 = 0;
    int i = 0;
    QVector<_valueWithFitness>* Bestsolutions = new QVector<_valueWithFitness>(archive->size());

    while ( i< archive->size() && i2 < nbAnt)
    {
        if ((*archive)[i1].fitness > (*generatedSolutions)[i2].fitness)
        {
            (*Bestsolutions)[i]= (*archive)[i1];
            i1=i1+1;
        }
        else
        {
            (*Bestsolutions)[i]= (*generatedSolutions)[i2];
            i2=i2+1;
        }
        i = i+1;
    }
    if (i2 > nbAnt)
    {
        while (i<archive->size())
        {
            (*Bestsolutions)[i]=(*archive)[i1];
            i1 = i1+1;
            i=i+1;
        }
    }
    delete archive;
    archive = Bestsolutions;
}
