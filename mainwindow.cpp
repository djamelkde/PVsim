#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_about.h"


void clear_layout(QLayout * l)
{
    QLayoutItem * child;
    while ((child = l->takeAt(0)))
    {
        if (child->layout())
        {
            clear_layout(child->layout());
        }
        else if (child->widget())
        {
            delete child->widget();
        }
        delete child;
    }
}


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    aboutUi(new Ui::Dialog),
    aboutDialog(new QDialog(this)),
    panneau_manager(nullptr),
    afficheur(nullptr),
    panneau(nullptr),
    climat(nullptr),
    irradiationFactors(QVector<double>(DEFAULT_GROUPS, 1.)),
    temperatureFactors(QVector<double>(DEFAULT_GROUPS, .0)),
    fctrsAfterPerturbation(new QVector<double>(DEFAULT_GROUPS, .9)),
    Perturbingtime(150),
    _fileDialogPath("."),
    _algorithme_index(2)
{
    ui->setupUi(this);
    aboutUi->setupUi(aboutDialog);
    aboutDialog->setWindowTitle("A propos...");
    loadModels();
    createConnections();
    showMaximized();

    ui->table_algo->setColumnWidth(0, 40);
    ui->table_algo->setColumnWidth(1, 140);
    ui->table_algo->setColumnWidth(2, 320);
    ui->table_algo->setColumnWidth(3, 120);
    ui->table_algo->setColumnWidth(6, 150);

    ui->algorithme->setCurrentIndex(_algorithme_index);
    ui->params_widget->setCurrentIndex(_algorithme_index);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::createConnections()
{
    connect(ui->modele_panneau, SIGNAL(activated(int)), this, SLOT(modelChanged(int)));
    connect(ui->modele_irradiation, SIGNAL(activated(int)), this, SLOT(modelIrradiationChanged(int)));
    connect(ui->modele_temperature, SIGNAL(activated(int)), this, SLOT(modelTemperatureChanged(int)));
    connect(ui->algorithme, SIGNAL(activated(int)), ui->params_widget, SLOT(setCurrentIndex(int)));
    QObject::connect(ui->valider_inputs, SIGNAL(clicked()),
                     this, SLOT(validateInformations()));
    connect(ui->stop_simulation, SIGNAL(clicked()), this, SLOT(arreterButtonClicked()));
    connect(ui->start_simulation, SIGNAL(clicked()), this, SLOT(startButtonClicked()));
    connect(ui->simulation_pas_a_pas, SIGNAL(clicked()), this, SLOT(pasApasButtonClicked()));
    connect(ui->valider_inputs, SIGNAL(clicked()), this, SLOT(validerButtonClicked()));
    connect(ui->annuler_inputs, SIGNAL(clicked()), this, SLOT(annulerButtonClicked()));
    connect(ui->remuse_simulation, SIGNAL(clicked()), this, SLOT(pasApasButtonClicked()));
    connect(ui->save_data, SIGNAL(clicked()), this, SLOT(sauvegarderButtonClicked()));
    connect(ui->view_results, SIGNAL(clicked()), this, SLOT(voireResultatsBouttonsClicked()));
    connect(ui->importData, SIGNAL(clicked()), this, SLOT(importerButtonClicked()));
    connect(ui->clear_all, SIGNAL(clicked()), this, SLOT(supprimerTousClicked()));
    connect(ui->save_image, SIGNAL(clicked()), this, SLOT(sauvegarderImageButtonClicked()));
    // menu
    connect(ui->actionA_propos_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(ui->actionQuitter, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(ui->actionImport_Fichier_Simulation, SIGNAL(triggered()), this, SLOT(importerButtonClicked()));
    connect(ui->actionSauvegarder_fichier_simulation, SIGNAL(triggered()), this, SLOT(sauvegarderButtonClicked()));
    // about
    connect(aboutUi->fermerButton, SIGNAL(clicked()), aboutDialog, SLOT(close()));
    connect(ui->actionAbout, SIGNAL(triggered()), aboutDialog, SLOT(exec()));
}

void MainWindow::validateInformations()
{
    mettreAjourGridPannel(ui->rows->value(),ui->cols->value());
    if (panneau_manager != nullptr)
    {
        delete panneau_manager;
        delete afficheur;
        delete controlleur;
    }

    ClimatSimulator::modele_climat *model_irradiation = new ClimatSimulator::modele_climat(ui->irradiation->value(),
                                                           (ClimatSimulator::ModeleVariation) ui->modele_irradiation->currentIndex());

    ClimatSimulator::modele_climat *model_tempurature = new ClimatSimulator::modele_climat(ui->temperature->value(),
                                                           (ClimatSimulator::ModeleVariation) ui->modele_temperature->currentIndex());

    int cells;
    int index = ui->modele_panneau->currentIndex();
    if (index < Panneau::_models.size())
    {
        cells = Panneau::_models.at(index).cells;
    }
    else
    {
        cells = 36;
    }

    climat = new ClimatSimulator(DEFAULT_GROUPS,model_irradiation,model_tempurature);
    for (int i(0); i < DEFAULT_GROUPS; ++i)
    {
        if (ui->modele_irradiation->currentIndex() == 2)    /* valeurs spécifiques */
        {
            climat->setIrradiationFactorPerGroup(i, irradiationFactors.at(i));
        }
        else if(ui->modele_irradiation->currentIndex() == 3)    /* autre configuration */
        {
            climat->setIrradiationFactorPerGroup(i, irradiationFactors.at(i));
            if(fctrsAfterPerturbation != nullptr){
                climat->setirradiationFactorsAfterTiming(*fctrsAfterPerturbation);
                climat->setPerturbingTime(Perturbingtime);
            }
        }

        if (ui->modele_temperature->currentIndex() == 2)    /* valeurs spécifiques */
        {
            climat->setTemperatureFactorPerGroup(i, temperatureFactors.at(i));
        }
    }

    Panneau::_pv_stc * _stc = new Panneau::_pv_stc(ui->modele_panneau->currentText(), ui->Voc->value(), ui->Isc->value(), ui->Vmpp->value(), ui->Impp->value(), cells, ui->Ki->value(), ui->Kv->value());
    panneau = new Panneau(ui->rows->value(),ui->cols->value(),_stc,climat);
    panneau_manager = new PanneauManager(ui, panneau, climat);

    if (ui->algorithme->currentIndex() == 0)    // P&O
    {
        imppt = new Perturb_and_Observe(ui->po_pas->value());
    }
    else if (ui->algorithme->currentIndex() == 1)   // IC
    {
        imppt = new ICMpptTracker(ui->ic_pas->value()) ;
    }
    else if (ui->algorithme->currentIndex() == 2)   // ACO
    {
        double max_tension = 0.99 * panneau->getVoc() * panneau_manager->getNss();
        imppt = new AcoMPPTTracker(ui->aco_nb_fourmis->value(),ui->aco_taille_arch->value(),ui->aco_max_iter->value(),max_tension,ui->aco_alpha->value());
    }
    else if (ui->algorithme->currentIndex() == 3)   // AG
    {
        double max_tension = 0.99 * panneau->getVoc() * panneau_manager->getNss();
        imppt = new Algorithme_Genetique(ui->ag_taille_pop->value(), ui->ag_max_iter->value(), max_tension, ui->ag_prob_xover->value(), ui->ag_prob_mut->value());
    }
    else if (ui->algorithme->currentIndex() == 4)   // PSO
    {
        double max_tension = 0.99 * panneau->getVoc() * panneau_manager->getNss();
        imppt = new PSOMpptTracker(ui->pso_nbParticule->value(), ui->pso_nbIter->value(), max_tension, ui->pso_c1->value(), ui->pso_c2->value(), ui->pso_w->value());
    }

    controlleur = new ControlleurMPPT(panneau_manager, imppt, ui->periode->value());
    afficheur = new Afficheur(ui,panneau_manager, controlleur);

    connect(panneau_manager, SIGNAL(dataChanged()), afficheur, SLOT(dessinerCourbe()));
    connect(climat, SIGNAL(changementClimatique()),this, SLOT(updateIrradiationLevel()));
    updateIrradiationLevel();

    connect(ui->start_simulation, SIGNAL(clicked()), controlleur, SLOT(startMPPT()));
    connect(ui->simulation_pas_a_pas, SIGNAL(clicked()), controlleur, SLOT(simuterPasApas()));
    connect(ui->start_simulation, SIGNAL(clicked()), climat, SLOT(startSimulation()));
    connect(ui->stop_simulation, SIGNAL(clicked()), climat, SLOT(stopSimulation()));
    connect(ui->stop_simulation, SIGNAL(clicked()), controlleur, SLOT(stopMPPT()));

    connect(ui->annuler_inputs, SIGNAL(clicked()), climat, SLOT(stopSimulation()));
    connect(ui->annuler_inputs, SIGNAL(clicked()), controlleur, SLOT(stopMPPT()));
    connect(ui->remuse_simulation, SIGNAL(clicked()), controlleur, SLOT(resumeMPPT()));
}

void MainWindow::modelChanged(int index)
{
    if (index < Panneau::_models.size())
    {
        ui->Voc->setValue(Panneau::_models[index].v_oc);
        ui->Isc->setValue(Panneau::_models[index].i_sc);
        ui->Vmpp->setValue(Panneau::_models[index].v_mpp);
        ui->Impp->setValue(Panneau::_models[index].i_mpp);
        ui->Ki->setValue(Panneau::_models[index].k_i);
        ui->Kv->setValue(Panneau::_models[index].k_v);

        ui->Voc->setEnabled(false);
        ui->Isc->setEnabled(false);
        ui->Vmpp->setEnabled(false);
        ui->Impp->setEnabled(false);
        ui->Ki->setEnabled(false);
        ui->Kv->setEnabled(false);
    }
    else if (index == Panneau::_models.size())    // valeurs spécifiques
    {
        ui->Voc->setEnabled(true);
        ui->Isc->setEnabled(true);
        ui->Vmpp->setEnabled(true);
        ui->Impp->setEnabled(true);
        ui->Ki->setEnabled(true);
        ui->Kv->setEnabled(true);
    }
    else    // nouveau modèle
    {
        ui->Voc->setEnabled(false);
        ui->Isc->setEnabled(false);
        ui->Vmpp->setEnabled(false);
        ui->Impp->setEnabled(false);
        ui->Ki->setEnabled(false);
        ui->Kv->setEnabled(false);

        QDialog * dialog = new QDialog(this);
        int fixedWidth = 60;

        // nom
        QLabel * lbl_nom = new QLabel("Nom du Modèle");
        QLineEdit * _nom = new QLineEdit();
        lbl_nom->setBuddy(_nom);
        // voc
        QLabel * lbl_voc = new QLabel("V<sub>oc</sub> (STC)");
        QDoubleSpinBox * _voc = new QDoubleSpinBox();
        QLabel * lbl_voc_unit = new QLabel("V");
        lbl_voc->setBuddy(_voc);
        _voc->setRange(ui->Voc->minimum(), ui->Voc->maximum());
        _voc->setValue(ui->Voc->value());
        _voc->setFixedWidth(fixedWidth);
        // isc
        QLabel * lbl_isc = new QLabel("I<sub>sc</sub> (STC)");
        QDoubleSpinBox * _isc = new QDoubleSpinBox();
        QLabel * lbl_isc_unit = new QLabel("A");
        lbl_isc->setBuddy(_isc);
        _isc->setRange(ui->Isc->minimum(), ui->Isc->maximum());
        _isc->setValue(ui->Isc->value());
        _isc->setFixedWidth(fixedWidth);
        // vmpp
        QLabel * lbl_vmpp = new QLabel("V<sub>MPP</sub> (STC)");
        QDoubleSpinBox * _vmpp = new QDoubleSpinBox();
        QLabel * lbl_vmpp_unit = new QLabel("V");
        lbl_vmpp->setBuddy(_vmpp);
        _vmpp->setRange(ui->Vmpp->minimum(), ui->Vmpp->maximum());
        _vmpp->setValue(ui->Vmpp->value());
        _vmpp->setFixedWidth(fixedWidth);
        // impp
        QLabel * lbl_impp = new QLabel("I<sub>MPP</sub> (STC)");
        QDoubleSpinBox * _impp = new QDoubleSpinBox();
        QLabel * lbl_impp_unit = new QLabel("V");
        lbl_impp->setBuddy(_impp);
        _impp->setRange(ui->Impp->minimum(), ui->Impp->maximum());
        _impp->setValue(ui->Impp->value());
        _impp->setFixedWidth(fixedWidth);
        // kv
        QLabel * lbl_kv = new QLabel("K<sub>v</sub>");
        QDoubleSpinBox * _kv = new QDoubleSpinBox();
        lbl_kv->setBuddy(_kv);
        _kv->setRange(ui->Kv->minimum(), ui->Kv->maximum());
        _kv->setValue(ui->Kv->value());
        _kv->setFixedWidth(fixedWidth);
        // ki
        QLabel * lbl_ki = new QLabel("K<sub>i</sub>");
        QDoubleSpinBox * _ki = new QDoubleSpinBox();
        lbl_ki->setBuddy(_ki);
        _ki->setRange(ui->Ki->minimum(), ui->Ki->maximum());
        _ki->setValue(ui->Ki->value());
        _ki->setFixedWidth(fixedWidth);
        // cells
        QLabel * lbl_cells = new QLabel("Nombre de cellules");
        QSpinBox * _cells = new QSpinBox();
        lbl_cells->setBuddy(_cells);
        _cells->setRange(10, 100);
        _cells->setValue(36);
        //_cells->setFixedWidth(fixedWidth);

        QDialogButtonBox * buttonbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

        QGridLayout * _layout = new QGridLayout();
        // first row
        _layout->addWidget(lbl_nom,       0, 0, 1, 2);
        _layout->addWidget(_nom,          0, 2, 1, 4);
        // second row
        _layout->addWidget(lbl_voc,       1, 0);
        _layout->addWidget(_voc,          1, 1);
        _layout->addWidget(lbl_voc_unit,  1, 2);
        _layout->addItem(new QSpacerItem(20, 20, QSizePolicy::Maximum, QSizePolicy::Minimum), 1, 3, 1, 1);
        _layout->addWidget(lbl_isc,       1, 4);
        _layout->addWidget(_isc,          1, 5);
        _layout->addWidget(lbl_isc_unit,  1, 6);
        // third row
        _layout->addWidget(lbl_vmpp,      2, 0);
        _layout->addWidget(_vmpp,         2, 1);
        _layout->addWidget(lbl_vmpp_unit, 2, 2);
        _layout->addItem(new QSpacerItem(20, 20, QSizePolicy::Maximum, QSizePolicy::Minimum), 2, 3, 1, 1);
        _layout->addWidget(lbl_impp,      2, 4);
        _layout->addWidget(_impp,         2, 5);
        _layout->addWidget(lbl_impp_unit, 2, 6);
        // forth row
        _layout->addWidget(lbl_kv,        3, 0);
        _layout->addWidget(_kv,           3, 1);
        _layout->addItem(new QSpacerItem(20, 20, QSizePolicy::Maximum, QSizePolicy::Minimum), 3, 3, 1, 1);
        _layout->addWidget(lbl_ki,        3, 4);
        _layout->addWidget(_ki,           3, 5);
        // fifth row
        _layout->addWidget(lbl_cells,     4, 0, 1, 2);
        _layout->addWidget(_cells,        4, 5);
        // sixth row
        _layout->addWidget(buttonbox,     5, 0, 1, 7);
        dialog->setLayout(_layout);

        connect(buttonbox, SIGNAL(accepted()), dialog, SLOT(accept()));
        connect(buttonbox, SIGNAL(rejected()), dialog, SLOT(reject()));

        dialog->setWindowTitle("Caractéristiques du Modèle...");
        _nom->setText("Nom du Modèle");
        _nom->setFocus();
        _nom->selectAll();

        if (dialog->exec() == QDialog::Accepted)
        {
            // insert model in models array and save file
            Panneau::_pv_stc model = Panneau::_pv_stc(_nom->text(),
                                                      _voc->value(), _isc->value(),
                                                      _vmpp->value(), _impp->value(),
                                                      _cells->value(),
                                                      _ki->value(), _kv->value());
            Panneau::addModele(model);
            // insert model in models combobox
            ui->modele_panneau->insertItem(Panneau::_models.size()-1, _nom->text());
            // insert model characteristics in fields
            ui->Voc->setValue(_voc->value());
            ui->Isc->setValue(_isc->value());
            ui->Vmpp->setValue(_vmpp->value());
            ui->Impp->setValue(_impp->value());
            ui->Kv->setValue(_kv->value());
            ui->Ki->setValue(_ki->value());
            // select model in combobox
            ui->modele_panneau->setCurrentIndex(Panneau::_models.size()-1);
        }
        else
        {
            ui->modele_panneau->setCurrentIndex(index-1);
            modelChanged(index-1);
        }
    }
}


void MainWindow::modelIrradiationChanged(int index)
{
    if (ClimatSimulator::CustomModel == (ClimatSimulator::ModeleVariation) index)
    {
        QDialog * dialog = new QDialog(this);
        QDoubleSpinBox irradiationEditors[DEFAULT_GROUPS];
        QGridLayout * layout = new QGridLayout();
        QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

        double irradiation[DEFAULT_GROUPS];

        for (int i(0); i < DEFAULT_GROUPS; ++i)
        {
            if (climat != nullptr)
            {
                irradiation[i] = climat->getIrradaitionFactor(i) * 100.;
            }
            else
            {
                irradiation[i] = irradiationFactors.at(i) * 100;
            }
            irradiationEditors[i].setRange(.0, 100.);
            irradiationEditors[i].setSingleStep(1.);
            irradiationEditors[i].setDecimals(0);
            irradiationEditors[i].setValue(irradiation[i]);
            layout->addWidget(new QLabel(QString("Groupe (%1)").arg(i+1)), i, 0);
            layout->addWidget(&irradiationEditors[i], i, 1);
            layout->addWidget(new QLabel(QString(" % ")), i, 2);
        }

        layout->addWidget(buttonBox, DEFAULT_GROUPS, 0, 1, 2);
        dialog->setLayout(layout);
        dialog->setWindowTitle("Irradiation par Groupe");

        connect(buttonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), dialog, SLOT(reject()));

        if (dialog->exec() == QDialog::Accepted)
        {
            for (int i(0); i < DEFAULT_GROUPS; ++i)
            {
                if (climat != nullptr)
                {
                    climat->setIrradiationFactorPerGroup(i, irradiationEditors[i].value() / 100.);
                }

                irradiationFactors[i] = irradiationEditors[i].value() / 100.;
            }
        }
        else
        {
            QComboBox * s = qobject_cast<QComboBox*>(sender());
            if (s)
            {
                s->setCurrentIndex(0);
            }
        }
    }
    else if (ClimatSimulator::OtherConfiguration == (ClimatSimulator::ModeleVariation) index){
        QDialog * dialog = new QDialog(this);
        QDoubleSpinBox irradiationEditors[DEFAULT_GROUPS];
        QSpinBox time;
        time.setRange(100, 200); time.setSingleStep(10); time.setValue(Perturbingtime);
        QBoxLayout * lyt = new QBoxLayout(QBoxLayout::TopToBottom);

        QBoxLayout * lytTop = new QBoxLayout(QBoxLayout::LeftToRight);
        lytTop->addWidget(new QLabel(QString("niveau d'irradiations après: ")));
        lytTop->addWidget(&time);
        lytTop->addWidget(new QLabel(QString(" ms")));
        QGridLayout * layout = new QGridLayout();
        QDialogButtonBox * buttonBx = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

        double factors[DEFAULT_GROUPS];

        for (int i(0); i < DEFAULT_GROUPS; ++i)
        {
            if (climat != nullptr)
            {
                factors[i] = climat->getIrradaitionFactor(i);
            }
            else
            {
                factors[i] = irradiationFactors.at(i) * 100;
            }
            irradiationEditors[i].setRange(.0, 100.);
            irradiationEditors[i].setSingleStep(1.);
            irradiationEditors[i].setDecimals(0);
            irradiationEditors[i].setValue((*fctrsAfterPerturbation)[i]*100.0);
            layout->addWidget(new QLabel(QString("Groupe (%1)").arg(i+1)), i, 0);
            layout->addWidget(&irradiationEditors[i], i, 1);
            layout->addWidget(new QLabel(QString(" % ")), i, 2);
        }

        layout->addWidget(buttonBx, DEFAULT_GROUPS, 0, 1, 2);
        lyt->addLayout(lytTop);
        lyt->addLayout(layout);
        dialog->setLayout(lyt);
        dialog->setWindowTitle("Irradiation par Groupe");
        connect(buttonBx, SIGNAL(accepted()), dialog, SLOT(accept()));
        connect(buttonBx, SIGNAL(rejected()), dialog, SLOT(reject()));
        if (dialog->exec() == QDialog::Accepted)
        {
            Perturbingtime = time.value();
            fctrsAfterPerturbation = new QVector<double>(DEFAULT_GROUPS, 1.0);
            for (int i(0); i < DEFAULT_GROUPS; ++i)
            {
                (*fctrsAfterPerturbation)[i]= irradiationEditors[i].value()/100.0;
                 irradiationFactors[i] = 0.8;
            }
            if(climat != nullptr){
                climat->setirradiationFactorsAfterTiming(*fctrsAfterPerturbation);
                climat->setPerturbingTime(Perturbingtime);
            }
        }
        else
        {
            QComboBox * s = qobject_cast<QComboBox*>(sender());
            if (s)
            {
                s->setCurrentIndex(0);
            }
        }
    }
}

void MainWindow::modelTemperatureChanged(int index)
{
    if (ClimatSimulator::CustomModel == (ClimatSimulator::ModeleVariation) index)
    {
        QDialog * dialog = new QDialog(this);
        QDoubleSpinBox temperatureEditors[DEFAULT_GROUPS];
        QGridLayout * layout = new QGridLayout();
        QDialogButtonBox * buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

        double temperature[DEFAULT_GROUPS];

        for (int i(0); i < DEFAULT_GROUPS; ++i)
        {
            if (climat != nullptr)
            {
                temperature[i] = climat->getTemperature(i);
            }
            else
            {
                temperature[i] = ui->temperature->value() + temperatureFactors.at(i) * TEMPERATURE_STEP;
            }
            temperatureEditors[i].setRange(temperature[i] - TEMPERATURE_STEP, temperature[i] + TEMPERATURE_STEP);
            temperatureEditors[i].setSingleStep(1.);
            temperatureEditors[i].setDecimals(1);
            temperatureEditors[i].setValue(temperature[i]);
            layout->addWidget(new QLabel(QString("Groupe (%1)").arg(i)), i, 0);
            layout->addWidget(&temperatureEditors[i], i, 1);
            layout->addWidget(new QLabel(QString(" °C ")), i, 2);
        }

        layout->addWidget(buttonBox, DEFAULT_GROUPS, 0, 1, 2);
        dialog->setLayout(layout);
        dialog->setWindowTitle("Temperature par Groupe");

        connect(buttonBox, SIGNAL(accepted()), dialog, SLOT(accept()));
        connect(buttonBox, SIGNAL(rejected()), dialog, SLOT(reject()));

        if (dialog->exec() == QDialog::Accepted)
        {
            for (int i(0); i < DEFAULT_GROUPS; ++i)
            {
                if (climat != nullptr)
                {
                    climat->setTemperatureFactorPerGroup(i, (temperatureEditors[i].value() - ui->temperature->value()) / TEMPERATURE_STEP);
                }
                temperatureFactors[i] = (temperatureEditors[i].value() - ui->temperature->value()) / TEMPERATURE_STEP;
            }
        }
        else
        {
            QComboBox * s = qobject_cast<QComboBox*>(sender());
            if (s)
            {
                s->setCurrentIndex(0);
            }
        }
    }
}

void MainWindow::mettreAjourGridPannel(int nss, int npp)
{
    QGridLayout* grid = ui->gridPanel;
    resetPanneau(grid);

    QSizePolicy sizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    QLabel*  label;

    for(int i = 0; i < nss; i++){
        for (int j = 0; j < npp; j++){
            label = new QLabel(ui->panneau);
            label->setEnabled(true);
            sizePolicy.setHeightForWidth(label->sizePolicy().hasHeightForWidth());
            label->setSizePolicy(sizePolicy);
            label->setCursor(QCursor(Qt::PointingHandCursor));
            label->setLineWidth(1);
            QPixmap imgCell = QPixmap(QString::fromUtf8(":/img/cell"));
            label->setPixmap(imgCell);
            label->setScaledContents(true);
            label->setAlignment(Qt::AlignCenter);
            label->setTextInteractionFlags(Qt::LinksAccessibleByMouse);
            grid->addWidget(label, i, j, 1, 1);
        }
    }
}

void MainWindow::resetPanneau(QGridLayout *grid)
{
    int n=grid->rowCount();
    int m=grid->columnCount();

    for(int k = 0; k < n; k++){
        for(int l=0; l< m;l++){
            grid->removeItem(grid->itemAtPosition(k,l));
        }
    }
}

void MainWindow::updateIrradiationLevel()
{
    int ng = DEFAULT_GROUPS;
    int np = panneau_manager->getNpp();
    int ligne =0;
    for(int id_groupe=0; id_groupe<ng; id_groupe++){
        int nbEltByGroupe = panneau->getModulesOfGroups()[id_groupe];
        int delta = int(climat->getIrradaitionFactorGroupe(id_groupe)*(MAX_CONSTAST-MIN_CONSTAST)+MIN_CONSTAST);
        for(int i=0;i<nbEltByGroupe;i++){
            for(int j=0; j<np; j++){
                updateContrastImage(ligne, j,delta);
            }
            ligne++;
        }
    }
}

void MainWindow::updateContrastImage(int l, int c, int delta )
{
    QLabel * label= qobject_cast<QLabel*>(ui->gridPanel->itemAtPosition(l,c)->widget());
    QImage image = QImage(QString::fromUtf8(":/img/cell"));
    QColor oldColor;
    int r,g,b;
    for(int x=0; x<image.width(); x++){
        for(int y=0; y<image.height(); y++){
            oldColor = QColor(image.pixel(x,y));
            r = oldColor.red() + delta;
            g = oldColor.green() + delta;
            b = oldColor.blue() + delta;

            //we check if the new values are between 0 and 255
            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);

            image.setPixel(x,y, qRgb(r,g,b));
        }
    }
    label->setPixmap(QPixmap::fromImage(image));
    ui->gridPanel->removeItem(ui->gridPanel->itemAtPosition(l,c));
    ui->gridPanel->addWidget(label,l,c,1,1);
}

void MainWindow::annulerButtonClicked()
{
    ui->annuler_inputs->setEnabled(false);
    ui->valider_inputs->setEnabled(true);
    ui->start_simulation->setEnabled(false);
    ui->simulation_pas_a_pas->setEnabled(false);
    ui->stop_simulation->setEnabled(false);
    ui->remuse_simulation->setEnabled(false);
    ui->save_data->setEnabled(false);
    ui->view_results->setEnabled(false);
    setEnableInputs(true);

    if (panneau_manager != nullptr)
    {
        delete panneau_manager;
        delete afficheur;
        delete controlleur;

        panneau_manager = nullptr;
        afficheur = nullptr;
        controlleur = nullptr;
        panneau = nullptr;
        climat = nullptr;
    }
}

void MainWindow::setEnableInputs(bool enabled){
    ui->modele_panneau->setEnabled(enabled);

    if (!enabled || ui->modele_panneau->currentIndex() >= Panneau::_models.size())
    {
        ui->Isc->setEnabled(enabled);
        ui->Impp->setEnabled(enabled);
        ui->Voc->setEnabled(enabled);
        ui->Vmpp->setEnabled(enabled);
        ui->Ki->setEnabled(enabled);
        ui->Kv->setEnabled(enabled);
    }

    ui->cols->setEnabled(enabled);
    ui->rows->setEnabled(enabled);
    ui->temperature->setEnabled(enabled);
    ui->irradiation->setEnabled(enabled);
    ui->modele_irradiation->setEnabled(enabled);
    ui->modele_temperature->setEnabled(enabled);
    ui->periode->setEnabled(enabled);
    ui->algorithme->setEnabled(enabled);
    ui->po_pas->setEnabled(enabled);
    ui->ic_pas->setEnabled(enabled);
    ui->aco_max_iter->setEnabled(enabled);
    ui->aco_nb_fourmis->setEnabled(enabled);
    ui->aco_taille_arch->setEnabled(enabled);
    ui->aco_prob_evap->setEnabled(enabled);
    ui->aco_alpha->setEnabled(enabled);
    ui->ag_max_iter->setEnabled(enabled);
    ui->ag_prob_mut->setEnabled(enabled);
    ui->ag_prob_xover->setEnabled(enabled);
    ui->ag_taille_pop->setEnabled(enabled);
    ui->pso_nbIter->setEnabled(enabled);
    ui->pso_nbParticule->setEnabled(enabled);
    ui->pso_c1->setEnabled(enabled);
    ui->pso_c2->setEnabled(enabled);
    ui->pso_w->setEnabled(enabled);
}

void MainWindow::pasApasButtonClicked()
{

}

void MainWindow::validerButtonClicked(){
    ui->annuler_inputs->setEnabled(true);
    ui->valider_inputs->setEnabled(false);
    ui->start_simulation->setEnabled(true);
    ui->simulation_pas_a_pas->setEnabled(true);
    ui->stop_simulation->setEnabled(false);
    ui->remuse_simulation->setEnabled(false);
    setEnableInputs(false);
}

void MainWindow::arreterButtonClicked(){
    ui->start_simulation->setEnabled(false);
    ui->simulation_pas_a_pas->setEnabled(false);
    ui->stop_simulation->setEnabled(false);
    ui->remuse_simulation->setEnabled(false);
    ui->save_data->setEnabled(true);
    ui->view_results->setEnabled(true);
    ui->actionSauvegarder_fichier_simulation->setEnabled(true);
}

void MainWindow::sauvegarderButtonClicked()
{
    QString nom_fichier = QFileDialog::getSaveFileName(
                this, "Sauvegarder fichier", _fileDialogPath, "CSV File (*.csv)");
    if (!nom_fichier.isEmpty())
    {
        int idx = nom_fichier.lastIndexOf("/");
        if (idx >= 0)
        {
            _fileDialogPath = nom_fichier;
            _fileDialogPath.truncate(idx+1);
        }
        // save file
        if (!nom_fichier.endsWith(".csv"))
        {
            nom_fichier += ".csv";
        }

        QFile file(nom_fichier);
        if (!file.open(QFile::WriteOnly|QFile::Text))
        {
            qDebug() << "failed to save file!";
            return;
        }

        QTextStream out(&file);
        out.setCodec("UTF-8");

        QString _algorithmes[] = {"Perturb and Observe",
                                  "Incremental Conductance",
                                  "Ant Colony Optimization",
                                  "Algorithme Génétique"};
        QString _temperature[] = {"conditions stables",
                                  "conditions changeantes",
                                  "valeurs différentes"};
        QString _irradiation[] = {"conditions stables",
                                  "conditions changenantes",
                                  "conditions ombragées"};

        QString modele = ui->modele_panneau->currentText();
        QString algorithme = _algorithmes[ui->algorithme->currentIndex()];
        QString temperature = _temperature[ui->modele_temperature->currentIndex()];
        QString irradiation = _irradiation[ui->modele_irradiation->currentIndex()];

        QString parametres = "----";
        if (ui->algorithme->currentIndex() == 0) // P&O
        {
            parametres = QString("pas de déplacement (%1)").arg(ui->po_pas->value());
        }
        else if (ui->algorithme->currentIndex() == 1) // IC
        {
            //
        }
        else if (ui->algorithme->currentIndex() == 2) // ACO
        {
            parametres  = QString("taille de l'archive (%1), ").arg(ui->aco_taille_arch->value());
            parametres += QString("nombre de fourmis (%1), ").arg((ui->aco_nb_fourmis->value()));
            parametres += QString("nombre maximal d'itérations (%1), ").arg(ui->aco_max_iter->value());
            parametres += QString("taux d'evaporation (%1)").arg(ui->aco_prob_evap->value());
        }
        else if (ui->algorithme->currentIndex() == 3) // AG
        {
            parametres  = QString("taille de la population (%1), ").arg(ui->ag_taille_pop->value());
            parametres += QString("nombre maximal d'itérations (%1), ").arg((ui->ag_max_iter->value()));
            parametres += QString("probabilité de croisement (%1), ").arg(ui->ag_prob_xover->value());
            parametres += QString("probabilité de mutation (%1)").arg(ui->ag_prob_mut->value());
        }
        else if (ui->algorithme->currentIndex() == 4) // PSO
        {

        }

        QString periode = QString("%1").arg(ui->periode->value());
        QString temps_exec = ui->temps_exec->text();
        QString found_mpp = QString("%1").arg(ui->current_Pmpp->value());

        out << QString("Modèle du panneau          : ") << modele << "\n";
        out << QString("Algorithme                 : ") << algorithme << "\n";
        out << QString("Paramètres de l'algorithme : ") << parametres << "\n";
        out << QString("Température                : ") << ui->temperature->value()
            << QString(" °C, ") << temperature << "\n";
        out << QString("Irradiation                : ") << ui->irradiation->value()
            << QString(" W/m², ") << irradiation << "\n";
        out << QString("Période d'exécution        : ") << periode << " ms\n";
        out << QString("Temps d'exécution          : ") << temps_exec << " ms\n";
        out << QString("Puissance délivrée         : ") << found_mpp << " W\n";

        out << "t(ms),V(V),I(A),P(W)\n";

        for (int i(0); i < ui->data_execution->rowCount(); ++i)
        {
            out << ui->data_execution->item(i, 0)->text() << ",";
            out << ui->data_execution->item(i, 1)->text() << ",";
            out << ui->data_execution->item(i, 2)->text() << ",";
            out << ui->data_execution->item(i, 3)->text() << "\n";
        }

        file.close();
    }
}

void MainWindow::importerButtonClicked()
{
    QString nom_fichier = QFileDialog::getOpenFileName(
                this, "Importer fichier", _fileDialogPath, "CSV File (*.csv)");
    if (!nom_fichier.isEmpty())
    {
        int idx = nom_fichier.lastIndexOf("/");
        if (idx >= 0)
        {
            _fileDialogPath = nom_fichier;
            _fileDialogPath.truncate(idx+1);
        }
        // import
        QFile file(nom_fichier);
        if (!file.open(QFile::ReadOnly|QFile::Text))
        {
            // error
            return;
        }

        QTextStream in(&file);
        in.setCodec("UTF-8");

        // read general information
        QString modele = in.readLine().split(":").at(1).trimmed();
        QString algorithme = in.readLine().split(":").at(1).trimmed();
        QString parametres = in.readLine().split(":").at(1).trimmed();
        QString temperature = in.readLine().split(":").at(1).trimmed();
        QString irradiation = in.readLine().split(":").at(1).trimmed();
        QString periode = in.readLine().split(":").at(1).trimmed();
        QString temps_exec = in.readLine().split(":").at(1).trimmed().split(" ").at(0).trimmed();
        QString found_mpp = in.readLine().split(":").at(1).trimmed().split(" ").at(0).trimmed();

        int row = ui->table_algo->rowCount();

        ui->table_algo->insertRow(ui->table_algo->rowCount());

        qreal r = ((double)rand()) / RAND_MAX;
        qreal g = ((double)rand()) / RAND_MAX;
        qreal b = ((double)rand()) / RAND_MAX;

        QLabel * _clr = new QLabel();
        _clr->setStyleSheet(QString("background-color: rgb(%1,%2,%3);").arg(int(r*256)).arg(int(g*256)).arg(int(b*256)));

        ui->table_algo->setItem(row, 0, new QTableWidgetItem());
            ui->table_algo->setCellWidget(row, 0, _clr);
            ui->table_algo->item(row, 0)->setTextAlignment(Qt::AlignCenter);
        ui->table_algo->setItem(row, 1, new QTableWidgetItem(algorithme));
            ui->table_algo->item(row, 1)->setTextAlignment(Qt::AlignCenter);
        ui->table_algo->setItem(row, 2, new QTableWidgetItem(parametres));
            ui->table_algo->item(row, 2)->setTextAlignment(Qt::AlignCenter);
        ui->table_algo->setItem(row, 3, new QTableWidgetItem(modele));
            ui->table_algo->item(row, 3)->setTextAlignment(Qt::AlignCenter);
        ui->table_algo->setItem(row, 4, new QTableWidgetItem(temperature));
            ui->table_algo->item(row, 4)->setTextAlignment(Qt::AlignCenter);
        ui->table_algo->setItem(row, 5, new QTableWidgetItem(irradiation));
            ui->table_algo->item(row, 5)->setTextAlignment(Qt::AlignCenter);
        ui->table_algo->setItem(row, 6, new QTableWidgetItem(temps_exec));
            ui->table_algo->item(row, 6)->setTextAlignment(Qt::AlignCenter);
        ui->table_algo->setItem(row, 7, new QTableWidgetItem(found_mpp));
            ui->table_algo->item(row, 7)->setTextAlignment(Qt::AlignCenter);

        in.readLine(); // headers
        QStringList headers;
        headers << "t(ms)" << "V(V)" << "I(A)" << "P(W)";

        QTableWidget * table = new QTableWidget(0, headers.size());

        for (int i(0); i < headers.size(); ++i)
        {
            table->setHorizontalHeaderItem(i, new QTableWidgetItem(headers.at(i)));
            table->horizontalHeaderItem(i)->setTextAlignment(Qt::AlignCenter);
            table->horizontalHeader()->setStretchLastSection(true);
            table->horizontalHeader()->setDefaultSectionSize(100);
        }

        QGroupBox * gb = new QGroupBox(algorithme);
        QHBoxLayout * l = new QHBoxLayout();
        l->addWidget(table);
        gb->setLayout(l);
        ui->traceLayout->addWidget(gb);

        QString line;
        do
        {
            line = in.readLine();
            if (!line.isEmpty())
            {
                QStringList values = line.split(",");
                table->insertRow(table->rowCount());
                for (int i(0); i < table->columnCount(); ++i)
                {
                    table->setItem(table->rowCount()-1, i, new QTableWidgetItem(values.at(i)));
                    table->item(table->rowCount()-1, i)->setTextAlignment(Qt::AlignCenter);
                }
            }
        } while (!line.isEmpty());

        file.close();

        QColor color = QColor();
        color.setRgbF(r,g,b);
        dessinerGrapheComparaison(algorithme, color, table);
        ui->clear_all->setEnabled(true);
        ui->save_image->setEnabled(true);
        ui->tabWidget->setCurrentIndex(1);
    }
}

void MainWindow::supprimerTousClicked()
{
    ui->clear_all->setEnabled(false);
    ui->save_image->setEnabled(false);
    clear_layout(ui->traceLayout);
    while (ui->table_algo->rowCount() > 0)
    {
        ui->table_algo->removeRow(0);
    }
    ui->graphe_temps->detachItems();
    ui->graphe_temps->setAxisTitle(QwtPlot::xBottom, "");
    ui->graphe_temps->setAxisTitle(QwtPlot::yLeft, "");
}

void MainWindow::sauvegarderImageButtonClicked()
{
    QString nom_fichier = QFileDialog::getSaveFileName(
                this, "Sauvegarder image", _fileDialogPath, "Picture (*.png *.jpg *.jpeg *.bmp)");
    if (!nom_fichier.isEmpty())
    {
        int idx = nom_fichier.lastIndexOf("/");
        if (idx >= 0)
        {
            _fileDialogPath = nom_fichier;
            _fileDialogPath.truncate(idx+1);
        }
        // save picture
        if (!(nom_fichier.endsWith(".jpg") ||
              nom_fichier.endsWith(".jpeg") ||
              nom_fichier.endsWith(".png") ||
              nom_fichier.endsWith(".bmp")))
        {
            nom_fichier += ".png";
        }

        QPixmap pixmap = ui->graphe_temps->grab();
        if (!pixmap.save(nom_fichier))
        {
            qDebug() << "failed to save picture!";
        }
    }
}


void MainWindow::startButtonClicked()
{
    ui->start_simulation->setEnabled(false);
    ui->simulation_pas_a_pas->setEnabled(false);
    ui->stop_simulation->setEnabled(true);
    ui->remuse_simulation->setEnabled(true);
}

void MainWindow::voireResultatsBouttonsClicked()
{
    ui->save_image->setEnabled(true);
    ui->clear_all->setEnabled(true);
    ui->tabWidget->setCurrentIndex(1);

    QString _algorithmes[] = {"Perturb and Observe",
                              "Incremental Conductance",
                              "Ant Colony Optimization",
                              "Algorithme Génétique",
                              "Particle Swarme Optimization"};

    QString legend = _algorithmes[ui->algorithme->currentIndex()];

    copyDataToNewTab(legend);

    qreal r = ((double)rand()) / RAND_MAX;
    qreal g = ((double)rand()) / RAND_MAX;
    qreal b = ((double)rand()) / RAND_MAX;
    QColor color = QColor();
    color.setRgbF(r,g,b);

    dessinerGrapheComparaison(legend, color, ui->data_execution);
    saveResultsOfAlgorithme(legend, color);
}

void MainWindow::copyDataToNewTab(QString legend)
{
    QGroupBox * containerGroupe = new QGroupBox();
    containerGroupe->setTitle(legend);
    QTableWidget * table = new QTableWidget();
    QHBoxLayout * layout = new QHBoxLayout();
    containerGroupe->setLayout(layout);

    // set Header.
    table->horizontalHeader()->setStretchLastSection(true);
    table->setColumnCount(4);

    QTableWidgetItem * t1 = new QTableWidgetItem(); t1->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter|Qt::AlignCenter);
    t1->setText("t (ms)"); table->setHorizontalHeaderItem(0,t1);
    QTableWidgetItem * t2 = new QTableWidgetItem(); t2->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter|Qt::AlignCenter);
    t2->setText("V (V)");table->setHorizontalHeaderItem(1,t2);
    QTableWidgetItem * t3 = new QTableWidgetItem(); t3->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter|Qt::AlignCenter);
    t3->setText("I (I)"); table->setHorizontalHeaderItem(2,t3);
    QTableWidgetItem * t4 = new QTableWidgetItem(); t4->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter|Qt::AlignCenter);
    t4->setText("P (W)"); table->setHorizontalHeaderItem(3,t4);

    // copy data.
    table->setRowCount(ui->data_execution->rowCount());
    for(int row = 0; row< ui->data_execution->rowCount(); row++){
        table->setItem(row, 0, new QTableWidgetItem(*ui->data_execution->item(row, 0)));
        table->setItem(row, 1, new QTableWidgetItem(*ui->data_execution->item(row, 1)));
        table->setItem(row, 2, new QTableWidgetItem(*ui->data_execution->item(row, 2)));
        table->setItem(row, 3, new QTableWidgetItem(*ui->data_execution->item(row, 3)));
    }

    layout->addWidget(table);
    ui->traceLayout->addWidget(containerGroupe);
}

void MainWindow::dessinerGrapheComparaison(QString lgend, QColor color, QTableWidget * data)
{
    ui->graphe_temps->setCanvasBackground(QColor(Qt::white));
    QwtLegend *legend = new QwtLegend;
    legend->setFrameStyle(QFrame::Box|QFrame::Sunken);
    ui->graphe_temps->insertLegend(legend, QwtPlot::BottomLegend);

    QwtPlotCurve * ptime_curve = new QwtPlotCurve(lgend);
    QVector<double> timeData = QVector<double>(data->rowCount(), 0);
    QVector<double> ptimeData = QVector<double>(data->rowCount(), 0);

    // initialisation de la grille
    ui->graphe_temps->setAxisTitle(QwtPlot::yLeft, "Puissance [W]");
    ui->graphe_temps->setAxisTitle(QwtPlot::xBottom, "Temps [ms]");

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
    grid->attach(ui->graphe_temps);

    QPen vtimePen(Qt::SolidLine);
    vtimePen.setColor(color);
    vtimePen.setWidthF(1.8);
    ptime_curve->setPen(vtimePen);
    ptime_curve->setRenderHint(QwtPlotItem::RenderAntialiased, true);

    // get data.
    for(int i=0; i < data->rowCount(); i++)
    {
        timeData[i] = data->item(i,0)->text().toDouble();
        ptimeData[i] = data->item(i,1)->text().toDouble()*data->item(i,2)->text().toDouble();
    }

    //afficher le graphe.
    ptime_curve->setSamples(timeData, ptimeData);
    ptime_curve->attach(ui->graphe_temps);
    ui->graphe_temps->replot();
}

void MainWindow::saveResultsOfAlgorithme(QString legend, QColor color)
{
    ui->table_algo->setRowCount(ui->table_algo->rowCount()+1);

    QLabel * _clr = new QLabel();
    _clr->setStyleSheet(QString("background-color: rgb(%1,%2,%3);").arg(int(color.red())).arg(int(color.green())).arg(int(color.blue())));

    ui->table_algo->setItem(ui->table_algo->rowCount()-1, 0, new QTableWidgetItem());
        ui->table_algo->setCellWidget(ui->table_algo->rowCount()-1, 0, _clr);

    QTableWidgetItem * t1 = new QTableWidgetItem(); t1->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter|Qt::AlignCenter);
    t1->setText(legend); ui->table_algo->setItem(ui->table_algo->rowCount()-1, 1, t1);
    QTableWidgetItem * t2 = new QTableWidgetItem(); t2->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter|Qt::AlignCenter);

    // paramètres de l'algorithme.
    QString parametres = "----";
    if (ui->algorithme->currentIndex() == 0) // P&O
    {
        parametres = QString("pas de déplacement (%1)").arg(ui->po_pas->value());
    }
    else if (ui->algorithme->currentIndex() == 1) // IC
    {
        //
    }
    else if (ui->algorithme->currentIndex() == 2) // ACO
    {
        parametres  = QString("taille de l'archive (%1), ").arg(ui->aco_taille_arch->value());
        parametres += QString("nombre de fourmis (%1), ").arg((ui->aco_nb_fourmis->value()));
        parametres += QString("nombre maximal d'itérations (%1), ").arg(ui->aco_max_iter->value());
        parametres += QString("taux d'evaporation (%1)").arg(ui->aco_prob_evap->value());
    }
    else if (ui->algorithme->currentIndex() == 3) // AG
    {
        parametres  = QString("taille de la population (%1), ").arg(ui->ag_taille_pop->value());
        parametres += QString("nombre maximal d'itérations (%1), ").arg((ui->ag_max_iter->value()));
        parametres += QString("probabilité de croisement (%1), ").arg(ui->ag_prob_xover->value());
        parametres += QString("probabilité de mutation (%1)").arg(ui->ag_prob_mut->value());
    }
    else if (ui->algorithme->currentIndex() == 4) // PSO
    {
        parametres  = QString("Nombre de particules (%1), ").arg(ui->pso_nbParticule->value());
        parametres += QString("nombre maximal d'itérations (%1), ").arg((ui->pso_nbIter->value()));
        parametres += QString("W (%1), ").arg(ui->pso_w->value());
        parametres += QString("C1 (%1)").arg(ui->pso_c1->value());
        parametres += QString("C2 (%1)").arg(ui->pso_c2->value());
    }
    t2->setText(parametres);ui->table_algo->setItem(ui->table_algo->rowCount()-1, 2,t2);
    QTableWidgetItem * t3 = new QTableWidgetItem(); t3->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter|Qt::AlignCenter);
    t3->setText(ui->modele_panneau->currentText()); ui->table_algo->setItem(ui->table_algo->rowCount()-1, 3,t3);
    QTableWidgetItem * t4 = new QTableWidgetItem(); t4->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter|Qt::AlignCenter);
    QString irrad = QString(QString::number(ui->irradiation->value()));
    switch (ui->modele_irradiation->currentIndex()) {
    case 0:
        irrad.append(" W/m², conditions stables");
        break;
    case 1:
        irrad.append(" W/m², conditions changenantes");
        break;
    case 2:
        irrad.append(" W/m², conditions ombragées");
        break;
    case 3:
        irrad.append(" W/m², conditions changenantes");
        break;
    default:
        break;
    }
    t4->setText(irrad); ui->table_algo->setItem(ui->table_algo->rowCount()-1, 4,t4);
    QTableWidgetItem * t5 = new QTableWidgetItem(); t5->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter|Qt::AlignCenter);
    QString temperature = QString(QString::number(ui->temperature->value()));
    switch (ui->modele_irradiation->currentIndex()) {
    case 0:
        temperature.append(" °C, conditions stables");
        break;
    case 1:
        temperature.append(" °C, conditions changenantes");
        break;
    case 2:
        temperature.append(" °C, valeurs différentes");
        break;
    default:
        break;
    }
    t5->setText(temperature);ui->table_algo->setItem(ui->table_algo->rowCount()-1, 5,t5);
    QTableWidgetItem * t6 = new QTableWidgetItem(); t6->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter|Qt::AlignCenter);
    t6->setText(ui->temps_exec->text()); ui->table_algo->setItem(ui->table_algo->rowCount()-1, 6,t6);
    QTableWidgetItem * t7 = new QTableWidgetItem(); t7->setTextAlignment(Qt::AlignHCenter|Qt::AlignVCenter|Qt::AlignCenter);
    t7->setText(QString::number(ui->current_Pmpp->value())); ui->table_algo->setItem(ui->table_algo->rowCount()-1, 7,t7);
}

QStringList * MainWindow::modelsNames()
{
    QStringList * list = new QStringList();

    for (int i(0); i < ui->modele_panneau->count(); ++i)
    {
        list->append(ui->modele_panneau->itemText(i));
    }

    return list;
}

void MainWindow::loadModels()
{
    Panneau::initModeles();

    for (int i(0); i < Panneau::_models.size(); ++i)
    {
        ui->modele_panneau->addItem(Panneau::_models.at(i).nom);
    }

    ui->modele_panneau->addItem("Modèle Spécifique");
    ui->modele_panneau->addItem("Nouveau Modèle...");
    ui->modele_panneau->setCurrentIndex(0);
    modelChanged(0);
}

//void MainWindow::updateValueIrradiationLevel(int val){
//    ui->valueIrrad->setText(QString("%1%").arg(val));
//    for(int i = 0; i < 4; i++){
//        climat->setIrradiationFactorPerGroup(i,val);
//    }
//}
