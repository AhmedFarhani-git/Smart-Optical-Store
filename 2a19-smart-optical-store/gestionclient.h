#ifndef GESTIONCLIENT_H
#define GESTIONCLIENT_H

#include <QString>
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QMap>
#include <QByteArray>

class GestionClient
{
private:
    int id_client;
    QString nom_client;
    QString prenom_client;
    QString email_client;
    QString telephone_client;
    QString adresse_client;
    QString date_naissance_client;
    int fidelite_client;
    QByteArray photo_client;  // BLOB PHOTO

public:
    // Constructeurs
    GestionClient();
    GestionClient(int, QString, QString, QString, QString, QString, QString, int);

    // Getters
    int getIdClient() const { return id_client; }
    QString getNomClient() const { return nom_client; }
    QString getPrenomClient() const { return prenom_client; }
    QString getEmailClient() const { return email_client; }
    QString getTelephoneClient() const { return telephone_client; }
    QString getAdresseClient() const { return adresse_client; }
    QString getDateNaissanceClient() const { return date_naissance_client; }
    int getFideliteClient() const { return fidelite_client; }
    QByteArray getPhotoClient() const { return photo_client; }

    // Setters
    void setIdClient(int id) { id_client = id; }
    void setNomClient(const QString &nom) { nom_client = nom; }
    void setPrenomClient(const QString &prenom) { prenom_client = prenom; }
    void setEmailClient(const QString &email) { email_client = email; }
    void setTelephoneClient(const QString &tel) { telephone_client = tel; }
    void setAdresseClient(const QString &adr) { adresse_client = adr; }
    void setDateNaissanceClient(const QString &date) { date_naissance_client = date; }
    void setFideliteClient(int f) { fidelite_client = f; }
    void setPhotoClient(const QByteArray &photo) { photo_client = photo; }

    // CRUD
    bool ajouter();
    QSqlQueryModel* afficher();
    bool supprimer(int);
    bool modifier(int, QString, QString, QString, QString, QString, QString, int);

    // Recherche & Tri
    QSqlQueryModel* rechercherParNom(const QString &nom);
    QSqlQueryModel* trierParOrdreAlphabetique(bool ascendant = true);
    QSqlQueryModel* trierParID(bool ascendant = true);

    // Statistiques
    QMap<QString, int> statistiquesTranchesAge();

    // Utilitaires
    bool idExists(int id);
    bool validerDate(const QString &dateStr);
    bool clientAvecAchats(int id);

    // Photos
    bool chargerPhotoDepuisFichier(const QString &cheminFichier);
};

#endif // GESTIONCLIENT_H
