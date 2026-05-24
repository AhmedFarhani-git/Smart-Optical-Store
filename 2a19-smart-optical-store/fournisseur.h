#ifndef FOURNISSEUR_H
#define FOURNISSEUR_H
#include <QString>
#include <QSqlQuery>
#include <QSqlQueryModel>

class Fournisseur
{
    QString nom, email, telephone, adresse, categorie;
    int id_fournisseur;

public:
    // constructeurs
    Fournisseur(){}
    Fournisseur(int id, QString nom, QString email, QString telephone, QString adresse, QString categorie);

    // getters
    QString getNom(){ return nom; }
    QString getEmail(){ return email; }
    QString getTelephone(){ return telephone; }
    QString getAdresse(){ return adresse; }
    QString getCategorie(){ return categorie; }
    int getID(){ return id_fournisseur; }

    // setters
    void setNom(QString n){ nom = n; }
    void setEmail(QString e){ email = e; }
    void setTelephone(QString t){ telephone = t; }
    void setAdresse(QString a){ adresse = a; }
    void setCategorie(QString c){ categorie = c; }
    void setID(int id){ id_fournisseur = id; }

    // CRUD
    bool ajouter();
    bool supprimer(int id);
    bool modifier(int id);
    QSqlQueryModel* afficher();

    //recherche
    QSqlQueryModel* rechercher(const QString &critere);
    //tri
    QSqlQueryModel* trier(const QString &critere);
    //export-pdf
    bool exporterPDF(const QString &filePath);
    // --- Statistiques ---
    QSqlQueryModel* statistiquesParCategorie();
    // --- Métier avancé : détection d'anomalies ---
    bool envoyerEmailAlerte(QString email, QString nom, QString message);



};

#endif // FOURNISSEUR_H
