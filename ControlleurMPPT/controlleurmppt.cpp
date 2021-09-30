    #include "controlleurmppt.h"

ControlleurMPPT::ControlleurMPPT(ICourant_Tension * icourant_tension, IMPPT * imppt, int periode)
    : icourant_tension(icourant_tension)
    , perdiodMPPT(periode)
    , imppt(imppt)
    , tempsExecution(0)

{
    connect(&timer, SIGNAL(timeout()), this, SLOT(trackMPP()));
}

ControlleurMPPT::~ControlleurMPPT()
{
    delete imppt;
}

unsigned long ControlleurMPPT::getTempsExecution() const
{
    return tempsExecution;
}

void ControlleurMPPT::trackMPP()
{
    QElapsedTimer t;
    t.start();
    /* start mppt algorithm */
    double v = icourant_tension->getControlleurTension();
    double c = icourant_tension->getControlleurCourant();
    double tension = imppt->searchMPP(v, c);
    /* end mppt */
    if (tempsExecution)
    {
        tempsExecution += t.nsecsElapsed() + perdiodMPPT*1000000;
    }
    else
    {
        tempsExecution += t.nsecsElapsed();
    }
    icourant_tension->forcerTensionGPV(tension);
}

void ControlleurMPPT::startMPPT()
{
    timer.start(perdiodMPPT);
}

void ControlleurMPPT::stopMPPT()
{
    timer.stop();
}

void ControlleurMPPT::resumeMPPT()
{
    if (timer.isActive())
    {
        timer.stop();
    }
    else
    {
        timer.start();
    }
}

void ControlleurMPPT::simuterPasApas()
{
    trackMPP();
}
