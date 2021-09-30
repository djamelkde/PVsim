#ifndef CLIMATSIMULATOR_H
#define CLIMATSIMULATOR_H

#include <QObject>
#include <QTimer>
#include <QVector>


#define MIN_IRRADIATION_STEP (-0.05)
#define MAX_IRRADIATION_STEP (0.05)

#define TEMPERATURE_STEP (5)

class ClimatSimulator : public QObject
{
    Q_OBJECT

public:
    enum ModeleVariation
    {
        ConstantValue,
        VariableModel,
        CustomModel,
        OtherConfiguration
    };

    typedef struct modele_climat
    {
        double valeur_globale;
        ModeleVariation modele_variation;

    public:
        modele_climat(double val, ModeleVariation mdl)
            : valeur_globale(val), modele_variation(mdl)
        {}
    } modele_climat;

public:
    explicit ClimatSimulator(int nombre_groupes,
        modele_climat *irradiation=new modele_climat(1000., ConstantValue),
        modele_climat *temperature=new modele_climat(25., ConstantValue),
        QObject *parent=0);
    ~ClimatSimulator();

    double getIrradaition(int groupe);
    double getIrradaitionFactor(int groupe);
    double getTemperature(int groupe);
    void setIrradiationFactorPerGroup(int i, double factor);
    void setTemperatureFactorPerGroup(int i, double factor);
    void setirradiationFactorsAfterTiming(QVector<double> factors);
    void setPerturbingTime(int time);

    double getIrradaitionFactorGroupe(int groupe_id);

public slots:
    void startSimulation();
    void stopSimulation();
    void changerIrradiation();
    void changerTemperature();
    void perturberIrradiation();

signals:
    void changementClimatique();

private:
    int nombre_groupes;

    modele_climat *temperature;
    modele_climat *irradiation;

    QVector<double> irradiationFactorPerGroup;
    QVector<double> temperatureFactorPerGroup;

    QTimer irradiationTimer;
    QTimer temperatureTimer;
    QTimer irradiationPerturbingTimer;

    QVector<double> irradiationFactorsAfterTiming;
    int timeAfterPerturbation;
};


#endif // CLIMATSIMULATOR_H
