#ifndef CONTROLLEURMPPT_H
#define CONTROLLEURMPPT_H

#include <QTimer>
#include <QTime>
#include <QThread>
#include <QElapsedTimer>

#include "../GPV/icourant_tension.h"
#include "impp.h"
#include "../Algorithme/imppt.h"

class ControlleurMPPT : public QObject, public IMPP
{
    Q_OBJECT

public:
    ControlleurMPPT(ICourant_Tension * icourant_tension, IMPPT * imppt, int perdiodMPPT=1);
    ~ControlleurMPPT();

    unsigned long getTempsExecution() const;

public slots:
    void startMPPT();
    void stopMPPT();
    void trackMPP();
    void resumeMPPT();
    void simuterPasApas();

private:
    ICourant_Tension * icourant_tension;
    IMPPT * imppt;
    QTimer timer;
    int perdiodMPPT;
    unsigned long tempsExecution;
};

#endif // CONTROLLEURMPPT_H
