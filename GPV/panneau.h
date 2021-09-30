#ifndef PANNEAU_H
#define PANNEAU_H

#include <QtCore>
#include <QObject>
#include <QVector>


#include "climatsimulator.h"


#define DEFAULT_GROUPS 4

/*constantes */
#define BOLTZMANN 1.3806503e-23
#define ELECTRON_CHARGE 1.60217646e-19
#define REF_KALVEN 273.15
#define TEMP_STDR 25.
#define IRRAD_STDR 1000.


/*
 *
 */
class Panneau : public QObject
{
    Q_OBJECT

public:

    /*
     * structure contenant les caractéristiques
     * des modules photovoltaïques.
     */
    typedef struct _pv_stc {
        QString nom;    // nom du modèle
        double v_oc;    // tension de circuit-ouvert
        double i_sc;    // courant de court-circuit
        double v_mpp;   // tension au MPP
        double i_mpp;   // courant au MPP
        double k_i;
        double k_v;
        int cells;
        double r_s;
        double r_p;
        double a_1;
        double a_2;
    public:
        _pv_stc()
            : nom("Modèle Standard")
            , v_oc(21.1), i_sc(3.8)
            , v_mpp(17.1), i_mpp(3.5)
            , k_i(0.003), k_v(-0.080)
            , cells(36)
            , r_s(0.34), r_p(164.585828)
            , a_1(1), a_2(1.2)
        {}

        _pv_stc(QString nom, double v_oc, double i_sc,
            double v_mpp, double i_mpp,
            int cells,
                double ki, double kv)
            : nom(nom)
            , v_oc(v_oc), i_sc(i_sc)
            , v_mpp(v_mpp), i_mpp(i_mpp)
            , k_i(ki), k_v(kv)
            , cells(cells)
            , r_s(0.34), r_p(164.585828)
            , a_1(1), a_2(1.2)
        {}
    } _pv_stc;

    static QVector<_pv_stc> _models;

    struct _pv_reel {
        double v_oc;
        double i_sc;
        double v_pv;
        double i_pv;
        public:
            _pv_reel(_pv_stc * _stc)
                : v_oc(_stc->v_oc), i_sc(_stc->i_sc)
                , v_pv(_stc->v_mpp), i_pv(0)
            {}
    };


public:
    explicit Panneau(int serie, int parallele,
                     _pv_stc * pv_stc, ClimatSimulator * climat=0,
                     QObject * parent=0);
    ~Panneau();

    /* récupérer les dimensions du panneau */
    int getSerie() const;
    int getParallele() const;

    /* récupérer les valeurs des caractéristiques dans les conditions STC */
    double getVoc_stc() const;
    double getIsc_stc() const;
    double getVmpp_stc() const;
    double getImpp_stc() const;

    /* accesseurs pour les valeurs des caractéristiques en temps réel */
    double getVoc() const;
    double getIsc() const;
    double getVpv() const;
    double getIpv() const;

    /* forcer une valeur spécifique de tension ou de courant */
    bool forcerVpv(double value);
    bool forcerIpv(double value);
    double trouverCourant(double tension, double i_min, double i_max);

    /* récupérer le courant et la puissance pour une valeur de tension */
    double getCourant(double tension) const;
    double getCourantGroupe(int id_groupe, double tension, double initial_value) const;
    double fonctionCaracteristique(double courant, double tension, int id_groupe) const;
    double fonctionCaracteristiquePrime(double courant, double tension, int id_groupe) const;

    double getTension(double courant) const;
    double getTensionGroupe(int id_groupe, double courant, double initial_value) const;
    double fonctionVI(double tension, double courant, int id_groupe) const;
    double fonctionVIPrime(double tension, double courant, int id_groupe) const;
    QVector<int> getModulesOfGroups() const;

    double getPuissance(double tension) const;

    void setClimat(ClimatSimulator * climat);

    static void initModeles();
    static void saveModeles();
    static const _pv_stc getModele(int indexModele);
    static void addModele(_pv_stc model);

    static const int nombre_groupes;

public slots:
    void mettreajourCourantTension();

signals:
    void structureChanged();

private:
    /* le nombre de modules en série et en parallèle. */
    int serie;
    int parallele;
    QVector<int> modules_par_groupe;

    /* les caractéristiques relatives au conditions STC */
    _pv_stc * pv_stc;

    /* les caractéristiques de fonctionnement actuel */
    _pv_reel * pv_reel;

    /*  */
    ClimatSimulator * climat;
};


#endif // PANNEAU_H
