#include "panneaumanager.h"


PanneauManager::PanneauManager(Ui::MainWindow * ui, Panneau *panneau, ClimatSimulator*climat)
    : ui(ui)
    , valuesIV(new QVector<_valuesIV>(NB_VALUES))
    , panneau(panneau)
    , climat(climat)
{
    initialiseValues();
    connect(this->climat, SIGNAL(changementClimatique()), this, SLOT(initialiseValues()));
    connect(this->panneau, SIGNAL(structureChanged()), this, SIGNAL(dataChanged()));
}

PanneauManager::~PanneauManager()
{
    delete climat;
    delete panneau;
    delete valuesIV;
}

/* les méthodes de l'interface ICourant_Tension */

double PanneauManager::getControlleurCourant() const
{
    return getCourant();
}

double PanneauManager::getControlleurTension() const
{
    return getTension();
}

double PanneauManager::getControlleurIsc() const
{
    return getIsc();
}

double PanneauManager::getControlleurVoc() const
{
    return getVoc();
}

void PanneauManager::forcerCourantGPV(double courant)
{
    panneau->forcerIpv(courant);
}

void PanneauManager::forcerTensionGPV(double tension)
{
    int a(0), b(valuesIV->size()-1), m((a+b)/2);

    if (tension > valuesIV->at(0).tension)
    {
        tension = valuesIV->at(0).tension;
    }
    if (tension < valuesIV->last().tension)
    {
        tension = valuesIV->last().tension;
    }

    do
    {
        if (valuesIV->at(a).tension == tension)
        {
            /* found */
            panneau->forcerIpv(valuesIV->at(a).courant);
            return;
        }
        if (valuesIV->at(b).tension == tension)
        {
            /* found */
            panneau->forcerIpv(valuesIV->at(b).courant);
            return;
        }
        if (valuesIV->at(m).tension == tension)
        {
            /* found */
            panneau->forcerIpv(valuesIV->at(m).courant);
            return;
        }
        else if (valuesIV->at(m).tension < tension)
        {
            /* go down side */
            b = m;
            m = (a + b) / 2;
        }
        else    // valuesIV->at(m).tension > tension
        {
            /* go top side */
            a = m;
            m = (a + b) / 2;
        }
    } while ((b-a) > 1);

    /* not found, search more deeply */
    double courant = panneau->trouverCourant(tension, valuesIV->at(a).courant, valuesIV->at(b).courant);
    panneau->forcerIpv(courant);
}

/* les méthodes de l'interfnace ICourbe */

const QVector<_valuesIV>* PanneauManager::getCourbeData() const
{
    return valuesIV;
}

double PanneauManager::getCourbeIsc() const
{
    return getIsc();
}

double PanneauManager::getCourbeVoc() const
{
    return getVoc();
}

int PanneauManager::getNss() const
{
    return panneau->getSerie();
}

int PanneauManager::getNpp() const
{
    return panneau->getParallele();
}

/* ----------------------------- */

double PanneauManager::getCourant() const
{
    return panneau->getIpv();
}

double PanneauManager::getCourant(double tension) const
{
    return panneau->getCourant(tension);
}

double PanneauManager::getTension() const
{
    return panneau->getVpv();
}

double PanneauManager::getPuissance(double tension) const
{
    return panneau->getPuissance(tension);
}

double PanneauManager::getIsc() const
{
    return panneau->getIsc();
}

double PanneauManager::getVoc() const
{
    return panneau->getVoc();
}

void PanneauManager::setPanneau(Panneau *panneau)
{
    this->panneau = panneau;
}

void PanneauManager::setClimat(ClimatSimulator *climat)
{
    this->climat = climat;
    this->panneau->setClimat(climat);
}

void PanneauManager::initialiseValues()
{
    double courant;
    double voltage;
    double max = panneau->getIsc()*getNpp();

    for(int i=0; i < valuesIV->size(); i++)
    {
         courant = i*max / valuesIV->size();
         voltage = panneau->getTension(courant);
         (*valuesIV)[i] = _valuesIV(voltage,courant);
    }

    emit dataChanged();
}

const QPointF PanneauManager::getCourbePoint() const
{
    return QPointF(panneau->getVpv(), panneau->getIpv());
}
