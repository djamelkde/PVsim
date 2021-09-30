#include "afficheur.h"

Afficheur::Afficheur(Ui::MainWindow * ui, const ICourbe * icourbe, const IMPP * impp, const int nombrePoints)
    : ui(ui)
    , iv_curve(new QwtPlotCurve("Courbe I/V"))
    , pv_curve(new QwtPlotCurve("Courbe P/V"))
    , iv_reel(nullptr)
    , icourbe(icourbe)
    , impp(impp)
    , xData(QVector<double>(nombrePoints, 0))
    , ivData(QVector<double>(nombrePoints, 0))
    , pvData(QVector<double>(nombrePoints, 0))
{
    initGraphe();
    dessinerCourbe();
}

void Afficheur::initGraphe()
{
    /* graphe */
    ui->graphe->detachItems(QwtPlotItem::Rtti_PlotItem);
    ui->graphe->setCanvasBackground(QColor(Qt::white));
    ui->graphe_temps->setCanvasBackground(QColor(Qt::white));
    ui->graphe->setAutoReplot(true);

    /* legend */
    QwtLegend *legend = new QwtLegend;
    legend->setFrameStyle(QFrame::Box|QFrame::Sunken);
    ui->graphe->insertLegend(legend, QwtPlot::BottomLegend);

    // top axis
    //ui->graphe->setAxisTitle(QwtPlot::xTop, "Caractéristiques I/V et P/V");
    // left axis
    ui->graphe->setAxisTitle(QwtPlot::yLeft, "Courant [A]");
    //ui->graphe->setAxisFont(QwtPlot::yLeft, QFont("Cambria", 13));
    // bottom axis
    ui->graphe->setAxisTitle(QwtPlot::xBottom, "Tension [V]");
    //ui->graphe->setAxisFont(QwtPlot::xBottom, QFont("Cambria", 13));
    // right axis
    ui->graphe->enableAxis(QwtPlot::yRight);
    ui->graphe->setAxisTitle(QwtPlot::yRight, "Puissance [W]");
    //ui->graphe->setAxisFont(QwtPlot::yRight, QFont("Cambria", 13));

    /* grid */
    QwtPlotGrid *grid = new QwtPlotGrid();

    QPen majPen = QPen();
    majPen.setWidthF(1.0);
    grid->setPen(majPen);
    QPen minPen = QPen();
    minPen.setWidthF(0.5);
    grid->setMinorPen(minPen);

    grid->enableX(true);
    grid->enableY(true);
    grid->enableXMin(true);
    grid->enableYMin(true);

    grid->attach(ui->graphe);

    /* iv_curve */
    QPen ivPen(Qt::SolidLine);
    ivPen.setColor(QColor(Qt::red));
    ivPen.setWidthF(2.0);
    iv_curve->setPen(ivPen);
    iv_curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    /* pv_curve */
    QPen pvPen(Qt::SolidLine);
    pvPen.setColor(QColor(Qt::blue));
    pvPen.setWidthF(2.0);
    pv_curve->setPen(pvPen);
    pv_curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);
    pv_curve->setYAxis(QwtPlot::yRight);

    iv_reel = new QwtPlotMarker();
    iv_reel->setSymbol(new QwtSymbol(QwtSymbol::Diamond, QBrush(Qt::yellow), QPen(Qt::red, 2), QSize(10, 10)));

    //
    while (ui->data_execution->rowCount() > 0)
    {
        ui->data_execution->removeRow(0);
    }
}

void Afficheur::dessinerCourbe()
{
    const QVector<_valuesIV>* data = icourbe->getCourbeData();
    QPointF iv_point = icourbe->getCourbePoint();
//    if(iv_point.x() == 0){ // tension = 0.0
//        iv_point.setX(qreal(ui->current_Vmpp->value()));
//        iv_point.setY(qreal(icourbe->getCourbeIsc()-1.0)*icourbe->getNpp());
//    }
//    if(iv_point.y() == 0){ // courant = 0.0
//        iv_point.setY(qreal(ui->current_Impp->value()));
//        iv_point.setX(qreal(icourbe->getCourbeVoc()-1.0)*icourbe->getNss());
//    }

    for(int i=xData.size()-1; i >= 0; i--)
    {
        ivData[i]= data->at(i).courant;
        xData[i] = data->at(i).tension;
        pvData[i] = xData[i]*ivData[i];
    }

    iv_curve->setSamples(xData, ivData);
    pv_curve->setSamples(xData, pvData);

    iv_reel->setValue(iv_point.x(), iv_point.x()*iv_point.y());
    iv_reel->setYAxis(QwtPlot::yRight);
    iv_reel->setLabel(QString("MPP"));

    iv_reel->attach(ui->graphe);
    iv_curve->attach(ui->graphe);
    pv_curve->attach(ui->graphe);

    ui->graphe->replot();

    unsigned long nsec = impp->getTempsExecution();
    double msec = (nsec * 1.) / 1000000;
    int i = ui->data_execution->rowCount();

    ui->temps_exec->setText(QString("%1").arg(msec));

    if (iv_point.x() > 0 && iv_point.y() > 0)
    {
        ui->current_Vmpp->display(iv_point.x());
        ui->current_Impp->display(iv_point.y());
        ui->current_Pmpp->display(iv_point.x() * iv_point.y());

        ui->data_execution->insertRow(i);
        ui->data_execution->setItem(i, 0, new QTableWidgetItem(QString("%1").arg(msec)));
            ui->data_execution->item(i, 0)->setTextAlignment(Qt::AlignCenter);
        ui->data_execution->setItem(i, 1, new QTableWidgetItem(QString("%1").arg(iv_point.x())));
            ui->data_execution->item(i, 1)->setTextAlignment(Qt::AlignCenter);
        ui->data_execution->setItem(i, 2, new QTableWidgetItem(QString("%1").arg(iv_point.y())));
            ui->data_execution->item(i, 2)->setTextAlignment(Qt::AlignCenter);
        ui->data_execution->setItem(i, 3, new QTableWidgetItem(QString("%1").arg(iv_point.x()*iv_point.y())));
            ui->data_execution->item(i, 3)->setTextAlignment(Qt::AlignCenter);
    }
    else
    {
        ui->current_Vmpp->display(0);
        ui->current_Impp->display(0);
        ui->current_Pmpp->display(0);

        if (msec > 0)
        {
            ui->data_execution->insertRow(i);
            ui->data_execution->setItem(i, 0, new QTableWidgetItem(QString("%1").arg(msec)));
                ui->data_execution->item(i, 0)->setTextAlignment(Qt::AlignCenter);
            ui->data_execution->setItem(i, 1, new QTableWidgetItem(QString("%1").arg(0)));
                ui->data_execution->item(i, 1)->setTextAlignment(Qt::AlignCenter);
            ui->data_execution->setItem(i, 2, new QTableWidgetItem(QString("%1").arg(0)));
                ui->data_execution->item(i, 2)->setTextAlignment(Qt::AlignCenter);
            ui->data_execution->setItem(i, 3, new QTableWidgetItem(QString("%1").arg(0)));
                ui->data_execution->item(i, 3)->setTextAlignment(Qt::AlignCenter);
        }
    }

    ui->data_execution->scrollToBottom();
}
