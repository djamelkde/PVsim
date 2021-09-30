#ifndef PANNEAUMANAGER_H
#define PANNEAUMANAGER_H

#include <QObject>

#include "icourbe.h"
#include "icourant_tension.h"
#include "panneau.h"
#include "climatsimulator.h"
#include "ui_mainwindow.h"




class PanneauManager : public QObject, public ICourbe, public ICourant_Tension
{
    Q_OBJECT
public:

    PanneauManager(Ui::MainWindow * ui, Panneau * panneau, ClimatSimulator * climat);
    ~PanneauManager();

    /* les métho des de l'interface ICourant_Tension */

    double getControlleurCourant() const;
    double getControlleurTension() const;
    double getControlleurIsc() const;
    double getControlleurVoc() const;
    void forcerCourantGPV(double);
    void forcerTensionGPV(double);

    /* les méthodes de l'interface ICourbe */

    const QVector<_valuesIV>* getCourbeData() const;
    const QPointF getCourbePoint() const;
    double getCourbeIsc() const;
    double getCourbeVoc() const;
    virtual int getNpp() const;
    virtual int getNss() const;

    void setPanneau(Panneau *panneau);
    void setClimat(ClimatSimulator* climat);

private:
    double getCourant() const;
    double getCourant(double) const;
    double getTension() const;
    double getPuissance(double) const;
    double getIsc() const;
    double getVoc() const;

public slots:
    void initialiseValues();

signals:
    void dataChanged();

private:
    Ui::MainWindow * ui;

    Panneau * panneau;
    ClimatSimulator * climat;
    QVector<_valuesIV> * valuesIV;
};

#endif // PANNEAUMANAGER_H
