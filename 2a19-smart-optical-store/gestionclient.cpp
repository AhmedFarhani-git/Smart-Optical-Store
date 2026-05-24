#include "gestionclient.h"
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QDebug>
#include <QDate>
#include <QRegularExpression>
#include <QFile>

GestionClient::GestionClient()
    : id_client(0), fidelite_client(0) {}

GestionClient::GestionClient(int id, QString nom, QString prenom, QString email, QString tel,
                             QString adresse, QString date_naissance, int fidelite)
    : id_client(id), nom_client(nom), prenom_client(prenom), email_client(email),
    telephone_client(tel), adresse_client(adresse), date_naissance_client(date_naissance),
    fidelite_client(fidelite)
{
}

// ====================== CHARGER PHOTO ======================
bool GestionClient::chargerPhotoDepuisFichier(const QString &cheminFichier)
{
    QFile file(cheminFichier);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "❌ Impossible d'ouvrir la photo:" << cheminFichier;
        return false;
    }

    photo_client = file.readAll();
    file.close();

    qDebug() << "✅ Photo chargée (" << photo_client.size() << " bytes )";
    return true;
}

// ====================== VALIDATION DATE ======================
bool GestionClient::validerDate(const QString &dateStr)
{
    if (dateStr.isEmpty()) return true;

    QRegularExpression regex("^\\d{2}/\\d{2}/\\d{4}$");
    if (!regex.match(dateStr).hasMatch()) return false;

    int j = dateStr.left(2).toInt();
    int m = dateStr.mid(3, 2).toInt();
    int a = dateStr.right(4).toInt();

    QDate d(a, m, j);
    return d.isValid() && d <= QDate::currentDate();
}

// ====================== ID EXISTS ======================
bool GestionClient::idExists(int id)
{
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM CLIENTS WHERE ID_CLIENT = :id");
    q.bindValue(":id", id);

    if (!q.exec()) return false;

    if (q.next()) return q.value(0).toInt() > 0;

    return false;
}

// ====================== CHECK IF CLIENT HAS SALES ======================
bool GestionClient::clientAvecAchats(int id)
{
    QSqlQuery q;
    q.prepare("SELECT COUNT(*) FROM VENTES WHERE ID_CLIENT = :id");
    q.bindValue(":id", id);

    if (!q.exec()) return false;

    if (q.next()) return q.value(0).toInt() > 0;

    return false;
}

// ====================== AJOUTER ======================
bool GestionClient::ajouter()
{
    if (idExists(id_client)) {
        qDebug() << "❌ ID existe déjà";
        return false;
    }

    if (nom_client.isEmpty() || prenom_client.isEmpty()) {
        qDebug() << "❌ Nom / Prénom obligatoires";
        return false;
    }

    if (!date_naissance_client.isEmpty() && !validerDate(date_naissance_client)) {
        qDebug() << "❌ Date invalide";
        return false;
    }

    QSqlQuery q;

    // Build query depending on fields
    QString sql =
        "INSERT INTO CLIENTS (ID_CLIENT, NOM, PRENOM, EMAIL, TELEPHONE, ADRESSE, DATE_NAISSANCE, FIDELITE, PHOTO) "
        "VALUES (:id, :nom, :prenom, :email, :tel, :adr, ";

    if (date_naissance_client.isEmpty())
        sql += "NULL";
    else
        sql += "TO_DATE(:dateN, 'DD/MM/YYYY')";

    sql += ", :fidelite, :photo)";

    q.prepare(sql);

    q.bindValue(":id", id_client);
    q.bindValue(":nom", nom_client);
    q.bindValue(":prenom", prenom_client);
    q.bindValue(":email", email_client);
    q.bindValue(":tel", telephone_client);
    q.bindValue(":adr", adresse_client);

    if (!date_naissance_client.isEmpty())
        q.bindValue(":dateN", date_naissance_client);

    q.bindValue(":fidelite", fidelite_client);
    q.bindValue(":photo", photo_client);

    if (!q.exec()) {
        qDebug() << "❌ Erreur ajout:" << q.lastError().text();
        return false;
    }

    qDebug() << "✅ Client ajouté";
    return true;
}

// ====================== AFFICHER ======================
QSqlQueryModel* GestionClient::afficher()
{
    auto *model = new QSqlQueryModel();

    model->setQuery("SELECT ID_CLIENT, NOM, PRENOM, EMAIL, TELEPHONE, ADRESSE, DATE_NAISSANCE, FIDELITE FROM CLIENTS");

    return model;
}

// ====================== SUPPRIMER ======================
bool GestionClient::supprimer(int id)
{
    if (clientAvecAchats(id)) {
        qDebug() << "❌ Client a des achats → suppression impossible";
        return false;
    }

    QSqlQuery q;
    q.prepare("DELETE FROM CLIENTS WHERE ID_CLIENT = :id");
    q.bindValue(":id", id);

    if (!q.exec()) return false;

    qDebug() << "✅ Client supprimé";
    return true;
}

// ====================== MODIFIER ======================
bool GestionClient::modifier(int id, QString nom, QString prenom, QString email,
                             QString tel, QString adr, QString dateN, int fid)
{
    if (!idExists(id)) return false;

    QSqlQuery q;

    QString sql =
        "UPDATE CLIENTS SET NOM=:nom, PRENOM=:prenom, EMAIL=:email, TELEPHONE=:tel, ADRESSE=:adr, "
        "FIDELITE=:fid, ";

    if (dateN.isEmpty())
        sql += "DATE_NAISSANCE=NULL";
    else
        sql += "DATE_NAISSANCE=TO_DATE(:dateN, 'DD/MM/YYYY')";

    sql += " WHERE ID_CLIENT=:id";

    q.prepare(sql);

    q.bindValue(":nom", nom);
    q.bindValue(":prenom", prenom);
    q.bindValue(":email", email);
    q.bindValue(":tel", tel);
    q.bindValue(":adr", adr);
    q.bindValue(":fid", fid);

    if (!dateN.isEmpty())
        q.bindValue(":dateN", dateN);

    q.bindValue(":id", id);

    return q.exec();
}

// ====================== RECHERCHE ======================
QSqlQueryModel* GestionClient::rechercherParNom(const QString &nom)
{
    auto *model = new QSqlQueryModel();

    QSqlQuery q;
    q.prepare("SELECT ID_CLIENT, NOM, PRENOM, EMAIL, TELEPHONE, ADRESSE, DATE_NAISSANCE, FIDELITE "
              "FROM CLIENTS WHERE UPPER(NOM) LIKE UPPER(:n) OR UPPER(PRENOM) LIKE UPPER(:n)");
    q.bindValue(":n", "%" + nom + "%");
    q.exec();

    model->setQuery(q);
    return model;
}

// ====================== TRI ======================
QSqlQueryModel* GestionClient::trierParOrdreAlphabetique(bool asc)
{
    auto *model = new QSqlQueryModel();
    model->setQuery(
        "SELECT ID_CLIENT, NOM, PRENOM, EMAIL, TELEPHONE, ADRESSE, DATE_NAISSANCE, FIDELITE "
        "FROM CLIENTS ORDER BY NOM " + QString(asc ? "ASC" : "DESC"));
    return model;
}

QSqlQueryModel* GestionClient::trierParID(bool asc)
{
    auto *model = new QSqlQueryModel();
    model->setQuery(
        "SELECT ID_CLIENT, NOM, PRENOM, EMAIL, TELEPHONE, ADRESSE, DATE_NAISSANCE, FIDELITE "
        "FROM CLIENTS ORDER BY ID_CLIENT " + QString(asc ? "ASC" : "DESC"));
    return model;
}

// ====================== STATISTIQUES ======================
QMap<QString, int> GestionClient::statistiquesTranchesAge()
{
    QMap<QString, int> stats;
    stats["Moins de 18 ans"] = 0;
    stats["18-25 ans"] = 0;
    stats["26-40 ans"] = 0;
    stats["41-60 ans"] = 0;
    stats["Plus de 60 ans"] = 0;

    QSqlQuery q("SELECT DATE_NAISSANCE FROM CLIENTS WHERE DATE_NAISSANCE IS NOT NULL");

    while (q.next()) {
        QString dateStr = q.value(0).toString();
        QDate d = QDate::fromString(dateStr, "dd/MM/yyyy");

        if (!d.isValid())
            d = QDate::fromString(dateStr, "dd/MM/yy");

        if (!d.isValid()) continue;

        int age = QDate::currentDate().year() - d.year();
        if (QDate::currentDate() < QDate(QDate::currentDate().year(), d.month(), d.day()))
            age--;

        if (age < 18) stats["Moins de 18 ans"]++;
        else if (age <= 25) stats["18-25 ans"]++;
        else if (age <= 40) stats["26-40 ans"]++;
        else if (age <= 60) stats["41-60 ans"]++;
        else stats["Plus de 60 ans"]++;
    }

    return stats;
}
