#ifndef LIGNEVENTE_H
#define LIGNEVENTE_H

#include <QString>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QTableWidget>

class LigneVente
{
private:
    int id_ligne;
    int id_vente;
    int id_produit;
    int quantite;
    double prix_unitaire;
    double remise_pourcent;

public:
    // Constructeurs
    LigneVente();
    LigneVente(int id_ligne, int id_vente, int id_produit, int quantite,
               double prix_unitaire, double remise_pourcent = 0.0);

    // Getters
    int getIdLigne() const { return id_ligne; }
    int getIdVente() const { return id_vente; }
    int getIdProduit() const { return id_produit; }
    int getQuantite() const { return quantite; }
    double getPrixUnitaire() const { return prix_unitaire; }
    double getRemisePourcent() const { return remise_pourcent; }
    double getPrixTotalLigne() const;

    // Setters
    void setIdLigne(int id) { id_ligne = id; }
    void setIdVente(int id) { id_vente = id; }
    void setIdProduit(int id) { id_produit = id; }
    void setQuantite(int q) { quantite = q; }
    void setPrixUnitaire(double p) { prix_unitaire = p; }
    void setRemisePourcent(double r) { remise_pourcent = r; }

    // CRUD
    bool ajouter();
    bool modifier(int id);
    bool supprimer(int id);
    QSqlQueryModel* afficher();
    QSqlQueryModel* afficherParVente(int id_vente);

    // Utilitaires
    bool verifierStockDisponible(int id_produit, int quantite);
    bool recupererPrixProduit(int id_produit, double &prix);
    QString getNomProduit(int id_produit);
};

#endif // LIGNEVENTE_H
