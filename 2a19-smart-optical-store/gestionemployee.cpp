#include "gestionemployee.h"
#include <QSqlQuery>
#include <QSqlQueryModel>
#include <QSqlError>
#include <QRegularExpression>
#include <QDebug>
#include <QDate>


gestionemployee::gestionemployee() : posteComboBox(nullptr) {}

void gestionemployee::setPosteComboBox(QComboBox *comboBox)
{
    posteComboBox = comboBox;  // Set the ComboBox pointer for later use
}
static inline QString normalizeEmail(const QString &raw)
{
    QString e = raw.trimmed();  // Remove leading and trailing spaces
    e.remove(QRegularExpression(R"(^\s*email\s*:\s*)", QRegularExpression::CaseInsensitiveOption)); // Clean unwanted patterns
    return e.toLower();  // Convert the email to lowercase
}

bool gestionemployee::ajouter(int id, QString nom, QString prenom, QString poste,
                              QString email, QString telephone, QString sexe,
                              QString situation, int enfants, QString mdp,
                              QString place, QString date, QString embauche_emp)
{
    QSqlQuery query;
    query.prepare(
        "INSERT INTO EMPLOYES "
        "(ID_EMPLOYE, NOM_EMPLOYE, PRENOM_EMPLOYE, POSTE_EMPLOYE, "
        "EMAIL_EMPLOYE, TELEPHONE_EMPLOYE, SEXE_EMPLOYE, "
        "SITUATION_FAMILIALE_EMPLOYE, NBR_ENFANTS_EMPLOYE, MDP_EMPLOYE, "
        "PLACE_EMPLOYE, DATE_EMPLOYE, EMBAUCHE_EMPLOYE) "
        "VALUES (:id, :nom, :prenom, :poste, :email, :telephone, :sexe, "
        ":situation, :enfants, :mdp, :place, :date, :embauche_emp)"
        );

    // Bind the values to the placeholders
    query.bindValue(":id", id);
    query.bindValue(":nom", nom.trimmed());
    query.bindValue(":prenom", prenom.trimmed());
    query.bindValue(":poste", poste.trimmed());
    query.bindValue(":email", normalizeEmail(email));  // Assuming normalizeEmail is a function to validate/format email
    query.bindValue(":telephone", telephone.trimmed());
    query.bindValue(":sexe", sexe.trimmed());
    query.bindValue(":situation", situation.trimmed());
    query.bindValue(":enfants", enfants);
    query.bindValue(":mdp", mdp.trimmed());
    query.bindValue(":place", place.trimmed());
    query.bindValue(":date", date.trimmed());
    query.bindValue(":embauche_emp", embauche_emp.trimmed());  // Store the hire date as a string

    // Execute the query
    if (!query.exec()) {
        qDebug() << "Erreur insertion employé:" << query.lastError().text();  // Log the error message
        return false;  // Return false if the query fails
    }

    return true;  // Return true if the employee was successfully added
}
bool gestionemployee::modifier(int id, QString nom, QString prenom, QString poste,
                               QString email, QString telephone, QString sexe,
                               QString situation, int enfants, QString mdp,
                               QString place, QString date, QString embauche_emp)
{
    qDebug() << "Updating Employee:";
    qDebug() << "ID: " << id;
    qDebug() << "Nom: " << nom;
    qDebug() << "Poste: " << poste;
    qDebug() << "Email: " << email;
    qDebug() << "Hire Date: " << embauche_emp;

    QSqlQuery query;
    query.prepare(
        "UPDATE EMPLOYES SET "
        "NOM_EMPLOYE = :nom, "
        "PRENOM_EMPLOYE = :prenom, "
        "POSTE_EMPLOYE = :poste, "
        "EMAIL_EMPLOYE = :email, "
        "TELEPHONE_EMPLOYE = :telephone, "
        "SEXE_EMPLOYE = :sexe, "
        "SITUATION_FAMILIALE_EMPLOYE = :situation, "
        "NBR_ENFANTS_EMPLOYE = :enfants, "
        "MDP_EMPLOYE = :mdp, "
        "PLACE_EMPLOYE = :place, "
        "DATE_EMPLOYE = :date, "
        "EMBAUCHE_EMPLOYE = :embauche_emp "
        "WHERE ID_EMPLOYE = :id"
        );

    // Bind the values to the placeholders
    query.bindValue(":id", id);
    query.bindValue(":nom", nom.trimmed());
    query.bindValue(":prenom", prenom.trimmed());
    query.bindValue(":poste", poste.trimmed());
    query.bindValue(":email", email.trimmed());
    query.bindValue(":telephone", telephone.trimmed());
    query.bindValue(":sexe", sexe.trimmed());
    query.bindValue(":situation", situation.trimmed());
    query.bindValue(":enfants", enfants);
    query.bindValue(":mdp", mdp.trimmed());
    query.bindValue(":place", place.trimmed());
    query.bindValue(":date", date.trimmed());
    query.bindValue(":embauche_emp", embauche_emp.trimmed());  // Pass the hire date as a string

    // Execute the query
    if (!query.exec()) {
        qDebug() << "Error updating employee:" << query.lastError().text();  // Log the error message
        return false;  // Return false if the query fails
    }

    return true;  // Return true if the employee was successfully updated
}




bool gestionemployee::supprimer(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM EMPLOYES WHERE ID_EMPLOYE = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Erreur suppression employé:" << query.lastError().text();
        return false;
    }
    return true;
}

QSqlQueryModel* gestionemployee::afficher()
{
    QSqlQueryModel *model = new QSqlQueryModel();

    QSqlQuery query;
    query.prepare(
        "SELECT "
        "ID_EMPLOYE, "
        "NOM_EMPLOYE, "
        "PRENOM_EMPLOYE, "
        "POSTE_EMPLOYE, "
        "EMAIL_EMPLOYE, "
        "TELEPHONE_EMPLOYE, "
        "SEXE_EMPLOYE, "
        "SITUATION_FAMILIALE_EMPLOYE, "
        "NBR_ENFANTS_EMPLOYE, "
        "MDP_EMPLOYE,"
        "PLACE_EMPLOYE, "
        "DATE_EMPLOYE, "
        "EMBAUCHE_EMPLOYE "  // Ensure this column is included
        "FROM EMPLOYES "
        "ORDER BY ID_EMPLOYE ASC"
        );

    if (!query.exec()) {
        qDebug() << "Erreur afficher EMPLOYES:" << query.lastError().text();
        return nullptr;
    }

    model->setQuery(query);
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Prénom"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Poste"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Email"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Téléphone"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Sexe"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Situation familiale"));
    model->setHeaderData(8, Qt::Horizontal, QObject::tr("Nb enfants"));
    model->setHeaderData(9, Qt::Horizontal, QObject::tr("Mot de passe"));
    model->setHeaderData(10, Qt::Horizontal, QObject::tr("Place de naissance"));
    model->setHeaderData(11, Qt::Horizontal, QObject::tr("Date de naissance"));
    model->setHeaderData(12, Qt::Horizontal, QObject::tr("Date d'embauche"));  // Header for hire date

    return model;
}




QSqlQueryModel* gestionemployee::rechercher(const QString &text)
{
    QSqlQueryModel *model = new QSqlQueryModel();

    QSqlQuery query;
    query.prepare("SELECT * FROM EMPLOYES "
                  "WHERE NOM_EMPLOYE LIKE :text "
                  "OR PRENOM_EMPLOYE LIKE :text "
                  "OR POSTE_EMPLOYE LIKE :text");

    query.bindValue(":text", "%" + text + "%");
    query.exec();

    model->setQuery(query);
    return model;
}
QSqlQueryModel* gestionemployee::trierAZ()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery("SELECT * FROM EMPLOYES ORDER BY NOM_EMPLOYE ASC");
    return model;
}
QSqlQueryModel* gestionemployee::trierAnciennete()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    QSqlQuery query;


    // Sort by "EMBAUCHE_EMPLOYE" using STR_TO_DATE to treat the string as a date
    query.prepare("SELECT * FROM EMPLOYES ORDER BY TO_DATE(EMBAUCHE_EMPLOYE, 'DD/MM/YYYY') DESC");



    if (!query.exec()) {
        qDebug() << "Erreur lors de la récupération des employés par date d'embauche:" << query.lastError().text();
        return nullptr;
    }

    model->setQuery(query);
    return model;
}


