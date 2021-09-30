#include <QTimer>

#include "climatsimulator.h"


ClimatSimulator::ClimatSimulator(int nombre_groupes,
                    modele_climat *irradiation, modele_climat *temperature,
                    QObject *parent)
    : QObject(parent)
    , nombre_groupes(nombre_groupes)
    , irradiation(irradiation)
    , temperature(temperature)
    , irradiationFactorPerGroup(QVector<double>(nombre_groupes, 1.0))
    , temperatureFactorPerGroup(QVector<double>(nombre_groupes, .0))
    , irradiationFactorsAfterTiming(QVector<double>(nombre_groupes, 1.0))
    , timeAfterPerturbation(100)
{
     connect(&irradiationTimer, SIGNAL(timeout()), this, SLOT(changerIrradiation()));
     connect(&temperatureTimer, SIGNAL(timeout()), this, SLOT(changerTemperature()));
     connect(&irradiationPerturbingTimer, SIGNAL(timeout()), this, SLOT(perturberIrradiation()));
}

ClimatSimulator::~ClimatSimulator()
{
    irradiationTimer.stop();
    temperatureTimer.stop();
    delete irradiation;
    delete temperature;
}

double ClimatSimulator::getIrradaition(int groupe)
{
    return irradiation->valeur_globale *
            irradiationFactorPerGroup.at(groupe);
}

double ClimatSimulator::getIrradaitionFactorGroupe(int groupe_id)
{
    Q_ASSERT(groupe_id >= 0 && groupe_id < nombre_groupes);
    return irradiationFactorPerGroup[groupe_id];
}

double ClimatSimulator::getIrradaitionFactor(int groupe)
{
    return irradiationFactorPerGroup.at(groupe);
}

double ClimatSimulator::getTemperature(int groupe)
{
    return temperature->valeur_globale +
            temperatureFactorPerGroup.at(groupe) * TEMPERATURE_STEP;
}

void ClimatSimulator::setIrradiationFactorPerGroup(int groupe_id, double factor)
{
    if (irradiationFactorPerGroup[groupe_id] != factor)
    {
        irradiationFactorPerGroup[groupe_id] = factor;
        emit changementClimatique();
    }
}

void ClimatSimulator::setTemperatureFactorPerGroup(int groupe_id, double factor)
{
    if (temperatureFactorPerGroup[groupe_id] != factor)
    {
        temperatureFactorPerGroup[groupe_id] = factor;
        emit changementClimatique();
    }
}

void ClimatSimulator::setirradiationFactorsAfterTiming(QVector<double> factors)
{
    if(factors.size() == nombre_groupes){
        irradiationFactorsAfterTiming = factors;
    }
}

void ClimatSimulator::setPerturbingTime(int time)
{
    timeAfterPerturbation = time;
}

void ClimatSimulator::changerTemperature()
{
    if (temperature->modele_variation == VariableModel)
    {
        double step = (double(qrand())/RAND_MAX) * 2. - 1.;
        for (int i(0); i < nombre_groupes; ++i)
        {
            double rand = double(qrand())/RAND_MAX;
            if (rand > 0.5)
            {
                temperatureFactorPerGroup[i] += step;
                temperatureFactorPerGroup[i] = temperatureFactorPerGroup.at(i) > 1. ? 1 : temperatureFactorPerGroup.at(i) < .0 ? 0 : temperatureFactorPerGroup.at(i);
            }
        }
        emit changementClimatique();
    }
}

void ClimatSimulator::perturberIrradiation()
{
    irradiationFactorPerGroup= irradiationFactorsAfterTiming;
    irradiationPerturbingTimer.stop();
    emit changementClimatique();
}

void ClimatSimulator::changerIrradiation()
{
    if (irradiation->modele_variation == VariableModel)
    {
        double step = (double(qrand())/RAND_MAX) * (MAX_IRRADIATION_STEP-MIN_IRRADIATION_STEP) + MIN_IRRADIATION_STEP;
        for (int i(0); i < nombre_groupes; ++i)
        {
            double rand = double(qrand())/RAND_MAX;
            if (rand > 0.5)
            {
                irradiationFactorPerGroup[i] += step;
                irradiationFactorPerGroup[i] = irradiationFactorPerGroup.at(i) > 1. ? 1 : irradiationFactorPerGroup.at(i) < .0 ? 0 : irradiationFactorPerGroup.at(i);
            }
        }
        emit changementClimatique();
    }
}


void ClimatSimulator::startSimulation()
{
    irradiationTimer.start(100);
    temperatureTimer.start(100);
    if (irradiation->modele_variation == OtherConfiguration){
        irradiationPerturbingTimer.start(timeAfterPerturbation*50);
    }
}

void ClimatSimulator::stopSimulation()
{
   irradiationTimer.stop();
   temperatureTimer.stop();
   irradiationPerturbingTimer.stop();
}
