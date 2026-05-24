#ifndef GESTIONVENTES_H
#define GESTIONVENTES_H

#include <QString>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QNetworkAccessManager>

class GestionVentes
{
    int id_vente;
    QString date_vente;
    double prix_ht;
    double tva;
    double remise;
    double prix_ttc;
    int id_client;
    int id_employe;
    QNetworkAccessManager *networkManager;
    void envoyerSMS(const QString &message);
    QString getNumeroResponsable();

public:
    // Constructeurs
    GestionVentes();
    GestionVentes(int, QString, double, double, double, double, int, int);
    ~GestionVentes();

    // Getters
    int getIdVente() { return id_vente; }
    QString getDateVente() { return date_vente; }
    double getPrixHT() { return prix_ht; }
    double getTVA() { return tva; }
    double getRemise() { return remise; }
    double getPrixTTC() { return prix_ttc; }
    int getIdClient() { return id_client; }
    int getIdEmploye() { return id_employe; }

    // Setters
    void setIdVente(int id) { id_vente = id; }
    void setDateVente(QString date) { date_vente = date; }
    void setPrixHT(double prix) { prix_ht = prix; }
    void setTVA(double t) { tva = t; }
    void setRemise(double r) { remise = r; }
    void setPrixTTC(double prix) { prix_ttc = prix; }
    void setIdClient(int id) { id_client = id; }
    void setIdEmploye(int id) { id_employe = id; }

    // Fonctionnalités CRUD
    bool ajouter();
    QSqlQueryModel* afficher();
    bool supprimer(int);
    bool modifier(int, QString, double, double, double, double, int, int);

    bool exporterPDF(const QString& cheminFichier);
    QSqlQueryModel* rechercherParDate(const QString &date);
    void notifierVente(const QString &action, int idVente, const QString &date,
                       const QString &categorie, int quantite, double remise, double prixTTC);

    // Méthodes pour les détails de vente (table CONTENIR)
    bool ajouterDetailVente(int idVente, int idProduit, int quantite, double prixUnitaire);
    QSqlQueryModel* afficherDetailsVente();
    bool supprimerDetailVente(int idVente, int idProduit);
    bool modifierDetailVente(int idVente, int idProduit, int nouvelleQuantite, double nouveauPrix);
    QSqlQueryModel* rechercherDetailsParVente(int idVente);
    QSqlQueryModel* rechercherDetailsParDate(const QString &date);
    double calculerSousTotal(int quantite, double prixUnitaire);
    bool exporterPDFDetails(const QString& cheminFichier);

    // NOUVELLES MÉTHODES POUR L'INTERFACE SIMPLIFIÉE
    bool ajouterDetailVenteAvecPrix(int idVente, int idProduit, int quantite);
};

#endif // GESTIONVENTES_H
