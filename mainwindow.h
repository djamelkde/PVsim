#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QLineEdit>

#include "GPV/panneaumanager.h"
#include "Afficheur/afficheur.h"
#include "GPV/panneau.h"
#include "ControlleurMPPT/controlleurmppt.h"
#include "Algorithme/imppt.h"
#include "Algorithme/perturb_and_observe.h"
#include "Algorithme/algorithme_genetique.h"
#include "Algorithme/acomppttracker.h"
#include "Algorithme/icmppttracker.h"
#include "Algorithme/psomppttracker.h"
#include "ui_about.h"

#define MAX_CONSTAST 100
#define MIN_CONSTAST -100


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    Ui::Dialog *aboutUi;
    QDialog * aboutDialog;
    PanneauManager * panneau_manager;
    Afficheur * afficheur;
    ControlleurMPPT * controlleur;
    Panneau * panneau;
    ClimatSimulator * climat;
    IMPPT * imppt;

    QVector<double> irradiationFactors;
    QVector<double> temperatureFactors;
    QVector<double> * fctrsAfterPerturbation;
    int Perturbingtime;

public:
    void createConnections();

    void copyDataToNewTab(QString legend);
    void dessinerGrapheComparaison(QString lgend, QColor color, QTableWidget *data);
    void saveResultsOfAlgorithme(QString legend, QColor color);
    QStringList * modelsNames();
    void loadModels();

public slots:
    void validateInformations();
    void modelChanged(int index);
    void modelIrradiationChanged(int index);
    void modelTemperatureChanged(int index);
    void mettreAjourGridPannel(int nss, int npp);
    void resetPanneau(QGridLayout *grid);
    void updateIrradiationLevel();
    void updateContrastImage(int x, int y, int Delta);
    void setEnableInputs(bool enabled);
    void annulerButtonClicked();
    void validerButtonClicked();
    void pasApasButtonClicked();
    void arreterButtonClicked();
    void sauvegarderButtonClicked();
    void importerButtonClicked();
    void supprimerTousClicked();
    void sauvegarderImageButtonClicked();
    void startButtonClicked();
    void voireResultatsBouttonsClicked();
private:
    QString _fileDialogPath;
    int _algorithme_index;
};

#endif // MAINWINDOW_H
