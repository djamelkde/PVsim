#ifndef AFFICHEUR_H
#define AFFICHEUR_H

#include <QVector>

#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_legend.h>
#include <qwt_plot_grid.h>
#include <qwt_symbol.h>
#include <qwt_plot_marker.h>
#include <qwt_text_label.h>

#include "ui_mainwindow.h"
#include "../GPV/icourbe.h"
#include "../ControlleurMPPT/impp.h"


class Afficheur : public QObject
{
    Q_OBJECT
public:
    Afficheur(Ui::MainWindow*, const ICourbe *, const IMPP *, const int = NB_VALUES);

public slots:
    void dessinerCourbe();

private:
    void initGraphe();

private:
    Ui::MainWindow * ui;

    QwtPlotCurve * iv_curve;
    QwtPlotCurve * pv_curve;
    QwtPlotMarker * iv_reel;

    const ICourbe * icourbe;
    const IMPP * impp;

    QVector<double> xData;
    QVector<double> ivData;
    QVector<double> pvData;
};

#endif // AFFICHEUR_H
