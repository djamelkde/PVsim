#include <cfloat>

#include "panneau.h"

QVector<Panneau::_pv_stc> Panneau::_models = QVector<Panneau::_pv_stc>();

const int Panneau::nombre_groupes = DEFAULT_GROUPS;


Panneau::Panneau(int serie, int parallele,
                 _pv_stc *pv_stc, ClimatSimulator * climat,
                 QObject *parent)
    : QObject(parent)
    , serie(serie)
    , parallele(parallele)
    , pv_stc(pv_stc)
    , climat(climat)
{
    this->pv_reel = new _pv_reel(pv_stc);
    this->pv_reel->v_pv = -1;
    this->pv_reel->i_pv = -1;

    if (serie < nombre_groupes)
    {
        serie = nombre_groupes;
    }

    modules_par_groupe = QVector<int>(nombre_groupes, serie/nombre_groupes);
    int rest = serie % nombre_groupes;

    for (int i(0); i < rest; ++i)
    {
        ++modules_par_groupe[i];
    }

    if (this->climat != nullptr)
    {
        connect(this->climat, SIGNAL(changementClimatique()),
                this, SLOT(mettreajourCourantTension()));
    }
}

Panneau::~Panneau()
{
    delete pv_stc;
    delete pv_reel;
}

/* le nombre de modules en série et en parallèle */

QVector<int> Panneau::getModulesOfGroups() const
{
    return modules_par_groupe;
}

int Panneau::getSerie() const
{
    return serie;
}

int Panneau::getParallele() const
{
    return parallele;
}

/* les caractéristiques dans les conditions standard (STC). */

double Panneau::getVoc_stc() const
{
    return pv_stc->v_oc;
}

double Panneau::getIsc_stc() const
{
    return pv_stc->i_sc;
}

double Panneau::getVmpp_stc() const
{
    return pv_stc->v_mpp;
}

double Panneau::getImpp_stc() const
{
    return pv_stc->i_mpp;
}

/* les caractéristiques de fonctionnement réel. */

double Panneau::getVoc() const
{
    return pv_reel->v_oc;
}

double Panneau::getIsc() const
{
    return pv_reel->i_sc;
}

double Panneau::getVpv() const
{
    return pv_reel->v_pv;
}

double Panneau::getIpv() const
{
    return pv_reel->i_pv;
}

/* forcer un valeur spécifique de tension */
bool Panneau::forcerVpv(double value)
{
    if (value != pv_reel->v_pv)
    {
        pv_reel->v_pv = value;

        emit structureChanged();
    }

    return true;
}

/* forcer un valeur spécifique de courant */
bool Panneau::forcerIpv(double value)
{
    if (value != pv_reel->i_pv)
    {
        pv_reel->i_pv = value;
        pv_reel->v_pv = getTension(value);

        emit structureChanged();
    }

    return true;
}

/* begin changes */
double Panneau::trouverCourant(double tension, double i_min, double i_max)
{
    double err = .001;
    double c((i_min+i_max)/2), dc(DBL_MAX), d;

    do
    {
        d = abs(getTension(i_min) - tension);
        if (d < dc)
        {
            c = i_min;
            dc = d;
        }

        d = abs(getTension(i_max) - tension);
        if (d < dc)
        {
            c = i_max;
            dc = d;
        }

        double m = (i_min + i_max) / 2;
        d = getTension(m) - tension;
        if (abs(d) < dc)
        {
            c = m;
            dc = abs(d);
        }

        if (d < 0)
        {
            i_max = m;
        }
        else
        {
            i_min = m;
        }
    } while (dc > err);

    return c;
}
/* end changes */

/* la valeur de courant pour une valeur de tension donnée */
double Panneau::getCourant(double tension) const
{
    double courant =0;
    double val = pv_stc->i_mpp;
    for(int i=0; i<nombre_groupes; i++){
        val=getCourantGroupe(i,tension,val);
        courant +=val;
    }
    return courant;
}

double Panneau::getCourantGroupe(int id_groupe, double tension, double initial_value) const
{
    //resolution de l'equatin I = f(I,V) par Newton Raphson.
    double courant = initial_value; // valeur initiale
    double seuil = 0.001;
    double error = abs(initial_value);
    double tmp_val = courant;
    while(error >= seuil){
        courant = courant - fonctionCaracteristique(courant,tension,id_groupe)/
                    fonctionCaracteristiquePrime(courant,tension,id_groupe);
        error = abs(courant-tmp_val);
        tmp_val = courant;
    }
    return courant;
}

double Panneau::fonctionCaracteristique(double courant,double tension,int id_groupe) const
{
    int nss_groupe = modules_par_groupe[id_groupe];
    double a1 = pv_stc->a_1;
    double a2 = pv_stc->a_2;
    double rs = pv_stc->r_s;
    double rp = pv_stc->r_p;
    double isc = pv_stc->i_sc;
    int ns = pv_stc->cells;
    double voc = pv_stc->v_oc;

    double dT = climat->getTemperature(id_groupe)-TEMP_STDR;
    double vt = ns*BOLTZMANN*(climat->getTemperature(id_groupe)+ REF_KALVEN)/ELECTRON_CHARGE;
    double i_0 =(isc+pv_stc->k_i*dT)/(exp((voc + pv_stc->k_v*dT)/(vt))-1);
    double i_diode1= i_0*(exp((tension+courant*rs*nss_groupe/parallele)/(a1*vt*nss_groupe))-1);
    double i_diode2= i_0*(exp((tension+courant*rs*nss_groupe/parallele)/(a2*vt*nss_groupe))-1);
    double i_pv = (isc+pv_stc->k_i*dT)*climat->getIrradaition(id_groupe)/IRRAD_STDR;
    double i_rest = (tension+courant*nss_groupe/parallele)/(rp*nss_groupe/parallele);
    return courant + parallele*(i_diode1+i_diode2-i_pv)+ i_rest;
}

double Panneau::fonctionCaracteristiquePrime(double courant, double tension, int id_groupe) const
{
    int nss_groupe = modules_par_groupe[id_groupe];
    double dT = climat->getTemperature(id_groupe)-TEMP_STDR;
    double vt = pv_stc->cells*BOLTZMANN*(climat->getTemperature(id_groupe)+ REF_KALVEN)/ELECTRON_CHARGE;
    double i_0 =(pv_stc->i_sc+pv_stc->k_i*dT)/(exp((pv_stc->v_oc + pv_stc->k_v*dT)/(vt))-1);
    double c1 = (pv_stc->r_s*nss_groupe/parallele)/(pv_stc->a_1*vt*nss_groupe);
    double c2 = (pv_stc->r_s*nss_groupe/parallele)/(pv_stc->a_2*vt*nss_groupe);
    double i_diode1= c1*(exp((tension+courant*pv_stc->r_s*nss_groupe/parallele)/(pv_stc->a_1*vt*nss_groupe)));
    double i_diode2= c2*(exp((tension+courant*pv_stc->r_s*nss_groupe/parallele)/(pv_stc->a_2*vt*nss_groupe)));
    double rest = 1+ pv_stc->r_s/pv_stc->r_p;
    return rest + parallele*i_0*(i_diode1 + i_diode2);
}

/*récupérer la valeur de la tension pour une valeur du courant*/
double Panneau::getTension(double courant) const
{
    double tension =0.0;
    double val = pv_stc->v_oc*modules_par_groupe[0];
    for(int i=0; i<nombre_groupes; i++){
        val = getTensionGroupe(i,courant,pv_stc->v_oc*modules_par_groupe[0]);
        tension += val > 0 ? val : 0;
    }
    return tension;
}

double Panneau::getTensionGroupe(int id_groupe, double courant, double initial_value) const
{
    double tension = initial_value; // valeur initiale
    double seuil = 0.001;
    double error = 100.;
    int iter=0;
    double g,gp;
    while(abs(error) >= seuil){
        g = fonctionVI(tension,courant,id_groupe);
        gp = fonctionVIPrime(tension,courant,id_groupe);
        error = g/gp;
        tension = tension - g/gp;
        iter++;
    }
    return tension;
}

double Panneau::fonctionVI(double tension, double courant, int id_groupe) const
{
    int _npp = parallele;
    int _nss = modules_par_groupe[id_groupe];;
    double a1 = pv_stc->a_1;
    double a2 = pv_stc->a_2;
    double rs = pv_stc->r_s*_nss/_npp;
    double rp = pv_stc->r_p*_nss/_npp;
    double dT = climat->getTemperature(id_groupe)-TEMP_STDR;
    double vt = pv_stc->cells*BOLTZMANN*(climat->getTemperature(id_groupe)+ REF_KALVEN)/ELECTRON_CHARGE;
    double i_0 = (pv_stc->i_sc+pv_stc->k_i*dT)/(exp((pv_stc->v_oc+pv_stc->k_v*dT)/vt)-1);
    double i_pv = (pv_stc->i_sc+pv_stc->k_i*dT)*climat->getIrradaition(id_groupe)/IRRAD_STDR;

    double i_diode1= i_0*(exp((tension+courant*rs)/a1/vt/_nss)-1);
    double i_diode2= i_0*(exp((tension+courant*rs)/a2/vt/_nss)-1);
    double i_rest = courant + (tension+courant*rs)/rp;
    double g = _npp*(i_diode1+i_diode2-i_pv)+ i_rest;

    return g;
}

double Panneau::fonctionVIPrime(double tension, double courant, int id_groupe) const
{
    int _npp = parallele;
    int _nss = modules_par_groupe[id_groupe];
    double a1 = pv_stc->a_1;
    double a2 = pv_stc->a_2;
    double rs = pv_stc->r_s*_nss/_npp;
    double rp = pv_stc->r_p*_nss/_npp;
    double dT = climat->getTemperature(id_groupe)-TEMP_STDR;
    double vt = pv_stc->cells*BOLTZMANN*(climat->getTemperature(id_groupe)+ REF_KALVEN)/ELECTRON_CHARGE;
    double i_0 = (pv_stc->i_sc+pv_stc->k_i*dT)/(exp((pv_stc->v_oc+pv_stc->k_v*dT)/(vt))-1);

    double gp = _npp * ((i_0/a1/vt/_nss)*exp((tension + courant*rs)/a1/vt/_nss) +
                    (i_0/a2/vt/_nss)*exp((tension + courant*rs)/a2/vt/_nss)) +
                    1/rp;
    return gp;
}

/* la valeur de puissance pour une valeur de tension donnée */
double Panneau::getPuissance(double tension) const
{
    return tension * getCourant(tension);
}

void Panneau::setClimat(ClimatSimulator *climat)
{
    if (this->climat != nullptr)
    {
        disconnect(this->climat, SIGNAL(changementClimatique()),
                        this, SLOT(mettreajourCourantTension()));
    }

    this->climat = climat;
    connect(this->climat, SIGNAL(changementClimatique()),
            this, SLOT(mettreajourCourantTension()));
}

void Panneau::initModeles()
{
    QFile file("models.cfg");
    if (file.exists() && file.open(QFile::ReadOnly|QFile::Text))
    {
        QTextStream in(&file);
        in.setCodec("UTF-8");
        QString line;
        do
        {
            line = in.readLine();
            if (!line.isEmpty())
            {
                QStringList fields = line.split(",");
                QString nom = fields.at(0);
                double voc = fields.at(1).toDouble();
                double isc = fields.at(2).toDouble();
                double vmpp = fields.at(3).toDouble();
                double impp = fields.at(4).toDouble();
                int cells = fields.at(5).toInt();
                double ki = fields.at(6).toDouble();
                double kv = fields.at(7).toDouble();
                Panneau::_models.append(Panneau::_pv_stc(nom, voc, isc, vmpp, impp, cells, ki, kv));
            }
        }
        while (!line.isEmpty());
        file.close();
    }
    else
    {
        Panneau::_models.append(Panneau::_pv_stc("BP MSX 60", 21.1, 3.8, 17.1, 3.5, 36, 0.003, -0.08));
        Panneau::_models.append(Panneau::_pv_stc("Siemens SM55", 21.7, 3.45, 17.4, 3.15, 36, .0012, -.077));
        Panneau::_models.append(Panneau::_pv_stc("Kyocera KG200GT", 32.9,8.21,26.3,7.61,54,.00318,-.123));
        Panneau::_models.append(Panneau::_pv_stc("Shell S36", 21.4,2.3,16.5,2.18,36,.001,-.076));
        Panneau::_models.append(Panneau::_pv_stc("Shell SP-70", 21.4,4.7,16.5,4.24,36,.002,-.076));
        Panneau::_models.append(Panneau::_pv_stc("Shell ST40", 23.3,2.68,16.6,2.41,42,.00035,-.1));
    }
}

void Panneau::saveModeles()
{
    QFile file("models.cfg");
    if (!file.open(QFile::WriteOnly|QFile::Text))
    {
        // error
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    for (int i(0); i < Panneau::_models.size(); ++i)
    {
        out << Panneau::_models.at(i).nom << ",";
        out << Panneau::_models.at(i).v_oc << ",";
        out << Panneau::_models.at(i).i_sc << ",";
        out << Panneau::_models.at(i).v_mpp << ",";
        out << Panneau::_models.at(i).i_mpp << ",";
        out << Panneau::_models.at(i).cells << ",";
        out << Panneau::_models.at(i).k_i << ",";
        out << Panneau::_models.at(i).k_v << "\n";
    }

    file.close();
}

const Panneau::_pv_stc Panneau::getModele(int indexModele)
{
    return _models.at(indexModele);
}

void Panneau::addModele(_pv_stc model)
{
    _models.append(model);
    Panneau::saveModeles();
}

void Panneau::mettreajourCourantTension()
{
    double tension = getTension(pv_reel->i_pv);

    if (tension != pv_reel->v_pv)
    {
        pv_reel->v_pv = tension;
        emit structureChanged();
    }
}
