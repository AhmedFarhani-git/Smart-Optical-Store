
#ifndef GESTIONSTOCK_H
#define GESTIONSTOCK_H

#include <QTableWidget>
#include <QComboBox>

class gestionstock
{
public:
    gestionstock() {}

    // Méthodes existantes
    bool ajouterProduit(const QString &nom, int quantite, const QString &marque,
                        const QString &categorie, double prix, int id_fournisseur);
    bool modifierProduit(int id, const QString &nom, int quantite, const QString &marque,
                         const QString &categorie, double prix, int id_fournisseur);
    bool supprimerProduit(int id);
    void exporterEnPDF(QTableWidget* table, const QString &nomFichier);

    void afficherProduits(QTableWidget* table);
    void rechercherProduit(QTableWidget* table, const QString &nom) const;
    void rechercherParNomOuId(QTableWidget* table, const QString &texte) const;
    void trierProduits(QTableWidget* table, const QString &critere, bool ordreCroissant) const;
    void mettreAJourStockSecurite(QTableWidget *table);

    // Nouvelles méthodes pour les combobox
    void remplirComboVentes(QComboBox* comboBox);
    void remplirComboProduits(QComboBox* comboBox);
    bool recupererRemiseVente(int id_vente, double &remise);
    bool recupererPrixProduit(int id_produit, double &prix);
    bool produitExiste(int id_produit);
    int getProduitIdByName(const QString &nomProduit);
    void remplirComboVentesAvecParDefaut(QComboBox* comboBox);
    void remplirComboProduitsAvecParDefaut(QComboBox* comboBox);

private:
    double rotationJournaliere(int idProduit, int jours = 30);
    double stockSecurite(int idProduit, int delaiAppro = 7, double facteur = 1.2);
};

#endif // GESTIONSTOCK_H
