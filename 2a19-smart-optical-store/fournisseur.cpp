#include "fournisseur.h"
#include <QSqlError>
#include <QDebug>
#include <QRegularExpression>
#include <QMessageBox>
#include <QPdfWriter>
#include <QPainter>
#include <QDateTime>
#include <QDir>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

Fournisseur::Fournisseur(int id, QString nom, QString email, QString telephone, QString adresse, QString categorie)
{
    this->id_fournisseur = id;
    this->nom = nom;
    this->email = email;
    this->telephone = telephone;
    this->adresse = adresse;
    this->categorie = categorie;
}

//-------------------------------------------------CRUD-------------------------------------------------------

// ---------------- AJOUTER ----------------
bool Fournisseur::ajouter()
{
    //Contrôle de saisie
    QRegularExpression regexNom("^[A-Za-zÀ-ÖØ-öø-ÿ\\s]+$");
    QRegularExpression regexTel("^\\d{8}$");
    QRegularExpression regexEmail("^[\\w\\.\\-]+@[\\w\\-]+\\.[A-Za-z]{2,}$");

    if (!regexNom.match(nom).hasMatch()) {
        QMessageBox::warning(nullptr, "Erreur", "Nom invalide — uniquement lettres ou espaces.");
        return false;
    }

    if (!regexTel.match(telephone).hasMatch()) {
        QMessageBox::warning(nullptr, "Erreur", "Téléphone invalide — doit contenir uniquement 8 chiffres (Tunisie).");
        return false;
    }

    if (!regexEmail.match(email).hasMatch()) {
        QMessageBox::warning(nullptr, "Erreur", "Email invalide — format attendu : exemple@mail.com");
        return false;
    }


    QSqlQuery query;
    //prepare() prend la requete en question pour la preparer a l'exec
    query.prepare("INSERT INTO fournisseurs (id_fournisseur, nom_fournisseur, email_fournisseur, telephone_fournisseur, adresse_fournisseur, categorie_fournisseur) "
                  "VALUES (:id, :nom, :email, :telephone, :adresse, :categorie)");
    query.bindValue(":id", id_fournisseur);
    query.bindValue(":nom", nom);
    query.bindValue(":email", email);
    query.bindValue(":telephone", telephone);
    query.bindValue(":adresse", adresse);
    query.bindValue(":categorie", categorie);

    return query.exec(); //envoie la requete pour l'executer
}

// ---------------- SUPPRIMER ----------------
bool Fournisseur::supprimer(int id)
{
    QSqlQuery check;
    check.prepare("SELECT COUNT(*) FROM fournisseurs WHERE id_fournisseur = :id");
    check.bindValue(":id", id);
    check.exec();
    check.next();

    if (check.value(0).toInt() == 0) {
        QMessageBox::warning(nullptr, "Erreur", "Suppression impossible : l'ID n'existe pas.");
        return false;
    }

    QSqlQuery query;
    QString res = QString::number(id);
    query.prepare("DELETE FROM fournisseurs WHERE id_fournisseur = :id");
    query.bindValue(":id", res);
    return query.exec();
}

// ---------------- MODIFIER ----------------
bool Fournisseur::modifier(int id)
{
    //check id exist
    QSqlQuery check;
    check.prepare("SELECT COUNT(*) FROM fournisseurs WHERE id_fournisseur = :id");
    check.bindValue(":id", id);
    check.exec();
    check.next();

    if (check.value(0).toInt() == 0) {
        QMessageBox::warning(nullptr, "Erreur", "Modification impossible : l'ID n'existe pas.");
        return false;
    }
    //Contrôle de saisie
    QRegularExpression regexNom("^[A-Za-zÀ-ÿ\\s]+$");  // lettres + espaces
    QRegularExpression regexTel("^\\d{8}$");           // 8 chiffres Tunisie
    QRegularExpression regexEmail("^[\\w\\.\\-]+@[\\w\\-]+\\.[A-Za-z]{2,}$");
    QRegularExpression regexAdresse("^[A-Za-z0-9À-ÖØ-öø-ÿ\\s,\\-]{3,}$");

    //Validation conditionnelle
    if (!nom.trimmed().isEmpty() && !regexNom.match(nom).hasMatch()) {
        QMessageBox::warning(nullptr, "Erreur", "Nom invalide — seules les lettres et espaces sont autorisés.");
        return false;
    }

    if (!telephone.trimmed().isEmpty() && !regexTel.match(telephone).hasMatch()) {
        QMessageBox::warning(nullptr, "Erreur", "Téléphone invalide — doit contenir 8 chiffres.");
        return false;
    }

    if (!email.trimmed().isEmpty() && !regexEmail.match(email).hasMatch()) {
        QMessageBox::warning(nullptr, "Erreur", "Email invalide — format attendu : exemple@mail.com");
        return false;
    }

    if (!adresse.trimmed().isEmpty() && !regexAdresse.match(adresse).hasMatch()) {
        QMessageBox::warning(nullptr, "Erreur", "Adresse invalide — minimum 3 caractères (lettres, chiffres, espaces).");
        return false;
    }
    QSqlQuery query;
    query.prepare(
        "UPDATE fournisseurs SET "
        "  nom_fournisseur = COALESCE(NULLIF(:nom, ''), nom_fournisseur),"
        "  email_fournisseur = COALESCE(NULLIF(:email, ''), email_fournisseur),"
        "  telephone_fournisseur = COALESCE(NULLIF(:telephone, ''), telephone_fournisseur),"
        "  adresse_fournisseur = COALESCE(NULLIF(:adresse, ''), adresse_fournisseur),"
        "  categorie_fournisseur = COALESCE(NULLIF(:categorie, ''), categorie_fournisseur)"
        " WHERE id_fournisseur = :id"
        );

    query.bindValue(":id", id);
    query.bindValue(":nom", nom.trimmed());
    query.bindValue(":email", email.trimmed());
    query.bindValue(":telephone", telephone.trimmed());
    query.bindValue(":adresse", adresse.trimmed());
    query.bindValue(":categorie", categorie.trimmed());

    return query.exec();
}

// ---------------- AFFICHER ----------------
QSqlQueryModel* Fournisseur::afficher()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery("SELECT id_fournisseur, nom_fournisseur, email_fournisseur, telephone_fournisseur, adresse_fournisseur, categorie_fournisseur FROM fournisseurs");

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Email"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Téléphone"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Adresse"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Catégorie"));

    return model;
}
// ---------------- RECHERCHER ----------------
QSqlQueryModel* Fournisseur::rechercher(const QString &critere)
{
    QSqlQueryModel *model = new QSqlQueryModel();
    QSqlQuery query;

    if (critere.trimmed().isEmpty()) {
        // Afficher tous les fournisseurs si aucun critère
        query.prepare("SELECT ID_FOURNISSEUR, NOM_FOURNISSEUR, EMAIL_FOURNISSEUR, TELEPHONE_FOURNISSEUR, ADRESSE_FOURNISSEUR, CATEGORIE_FOURNISSEUR "
                      "FROM FOURNISSEURS "
                      "ORDER BY NOM_FOURNISSEUR ASC");  // Tri par nom par défaut
    } else {
        // Recherche uniquement par Nom
        query.prepare("SELECT ID_FOURNISSEUR, NOM_FOURNISSEUR, EMAIL_FOURNISSEUR, TELEPHONE_FOURNISSEUR, ADRESSE_FOURNISSEUR, CATEGORIE_FOURNISSEUR "
                      "FROM FOURNISSEURS "
                      "WHERE LOWER(NOM_FOURNISSEUR) LIKE LOWER(:critere) "
                      "ORDER BY NOM_FOURNISSEUR ASC");  // Tri par nom
        query.bindValue(":critere", "%" + critere + "%");
    }

    query.exec();
    model->setQuery(query);

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Email"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Téléphone"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Adresse"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Catégorie"));

    return model;
}


// ---------------- TRI ----------------
QSqlQueryModel* Fournisseur::trier(const QString &critere)
{
    QSqlQueryModel *model = new QSqlQueryModel();
    QString queryStr;
    QString c = critere.trimmed().toLower(); // normalize input text

    if (c.contains("nom (a") || c.contains("nom a"))
        queryStr = "SELECT * FROM FOURNISSEURS ORDER BY NOM_FOURNISSEUR ASC";
    else if (c.contains("nom (z") || c.contains("nom z"))
        queryStr = "SELECT * FROM FOURNISSEURS ORDER BY NOM_FOURNISSEUR DESC";
    else if (c.contains("id croissant"))
        queryStr = "SELECT * FROM FOURNISSEURS ORDER BY ID_FOURNISSEUR ASC";
    else if (c.contains("id décroissant") || c.contains("id decroissant"))
        queryStr = "SELECT * FROM FOURNISSEURS ORDER BY ID_FOURNISSEUR DESC";
    else
        queryStr = "SELECT * FROM FOURNISSEURS";

    model->setQuery(queryStr);
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Email"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Téléphone"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Adresse"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Catégorie"));
    return model;
}

// ---------------- EXPORT-PDF ----------------

#include <QSqlQuery>
#include <QDateTime>
#include <QPainter>
#include <QPdfWriter>
#include <QPixmap>
#include <QPageSize>
#include <QString>
#include <QtCore/QDateTime>
#include <QColor>
bool Fournisseur::exporterPDF(const QString &filePath)
{
    QPdfWriter pdf(filePath);
    pdf.setPageSize(QPageSize(QPageSize::A4));
    pdf.setResolution(300);

    QPainter painter(&pdf);
    if (!painter.isActive())
        return false;

    // Fonts avec tailles ajustées pour les grandes lignes
    QFont titleFont("Chaparral Pro", 35, QFont::Bold);
    QFont headerFont("Arial", 14, QFont::Bold);
    QFont textFont("Arial", 9);
    QFont dateFont("Arial", 10);
    QFont pageNumberFont("Arial", 10);

    // Couleurs
    QColor titleColor(244, 143, 134);
    QColor headerBgColor(240, 240, 240);
    QColor borderColor(200, 200, 200);

    int y = 150;
    int pageNumber = 1;
    bool isFirstPage = true;

    // Fonction pour dessiner l'en-tête de page
    auto drawPageHeader = [&](int currentPage, bool firstPage) {
        if (firstPage) {
            // Logo et titre uniquement sur la première page
            QImage logo(":/images/ggggg.png");
            if (!logo.isNull()) {
                QSize logoSize(250, 250);
                int logoX = 2173;
                painter.drawImage(logoX, 10, logo.scaled(logoSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }

            // Titre principal centré
            painter.setFont(titleFont);
            painter.setPen(titleColor);
            QString title = "Liste des fournisseurs";
            painter.drawText(500, 150, title);

            // Date d'exportation alignée à droite
            QDateTime currentTime = QDateTime::currentDateTime();
            QString dateTime = currentTime.toString("dd/MM/yyyy HH:mm:ss");
            painter.setFont(dateFont);
            painter.setPen(Qt::black);
            painter.drawText(1728, 350, "Généré le : " + dateTime);
        }

        // Numéro de page en bas sur toutes les pages
        painter.setFont(pageNumberFont);
        painter.setPen(Qt::gray);
        painter.drawText(1240, 3430, QString::number(currentPage));
    };

    // Dessiner l'en-tête de la première page
    drawPageHeader(pageNumber, isFirstPage);

    y = 450; // Position Y après l'en-tête pour le tableau

    // Définition des colonnes
    int col1 = 20;    // ID
    int col2 = col1 + 150;   // Nom
    int col3 = col2 + 466;   //
    int col4 = col3 + 530;   // Téléphone
    int col5 = col4 + 300;
    int col6 = col5 + 300;


    int rowHeight = 100;
    int bottomMargin = 3400;

    // Fonction pour dessiner l'en-tête du tableau
    auto drawTableHeader = [&](int startY) {
        painter.setPen(QPen(borderColor, 2));
        painter.setFont(headerFont);
        painter.setBrush(headerBgColor);

        // Dessiner les cellules d'en-tête
        painter.drawRect(col1, startY, 150, rowHeight);
        painter.drawRect(col2, startY, 470, rowHeight);
        painter.drawRect(col3, startY, 530, rowHeight);
        painter.drawRect(col4, startY, 300, rowHeight);
        painter.drawRect(col5, startY, 300, rowHeight);
        painter.drawRect(col6, startY, 620, rowHeight);

        // Texte d'en-tête centré
        painter.setPen(Qt::black);

        QRect idHeaderRect(col1, startY, 150, rowHeight);
        painter.drawText(idHeaderRect, Qt::AlignCenter, "ID");

        QRect nomHeaderRect(col2, startY, 470, rowHeight);
        painter.drawText(nomHeaderRect, Qt::AlignCenter, "Nom");

        QRect prenomHeaderRect(col3, startY, 530, rowHeight);
        painter.drawText(prenomHeaderRect, Qt::AlignCenter, "Email");

        QRect telHeaderRect(col4, startY, 300, rowHeight);
        painter.drawText(telHeaderRect, Qt::AlignCenter, "Téléphone");

        QRect adresseHeaderRect(col5, startY, 300, rowHeight);
        painter.drawText(adresseHeaderRect, Qt::AlignCenter, "Adresse");

        QRect catHeaderRect(col6, y, 620, rowHeight);
        painter.drawText(catHeaderRect, Qt::AlignCenter, "Catégorie");
    };

    // Dessiner l'en-tête du tableau sur la première page
    drawTableHeader(y);
    y += rowHeight;

    // Data Query
    QSqlQuery query("SELECT ID_FOURNISSEUR, NOM_FOURNISSEUR, EMAIL_FOURNISSEUR, TELEPHONE_FOURNISSEUR, ADRESSE_FOURNISSEUR, CATEGORIE_FOURNISSEUR FROM FOURNISSEURS ORDER BY ID_FOURNISSEUR ASC");

    while (query.next()) {
        // Vérifier si on a besoin d'une nouvelle page
        if (y + rowHeight > bottomMargin) {
            pdf.newPage();
            pageNumber++;
            isFirstPage = false; // Ce n'est plus la première page

            // Dessiner seulement le numéro de page sur les pages suivantes
            drawPageHeader(pageNumber, isFirstPage);

            y = 100; // Commencer plus haut sur les pages suivantes

            // Dessiner l'en-tête du tableau sur la nouvelle page
            drawTableHeader(y);
            y += rowHeight;
        }

        // Dessiner les bordures des cellules de données
        painter.setPen(QPen(borderColor, 1));
        painter.setBrush(Qt::NoBrush);

        painter.drawRect(col1, y, 150, rowHeight);
        painter.drawRect(col2, y, 470, rowHeight);
        painter.drawRect(col3, y, 530, rowHeight);
        painter.drawRect(col4, y, 300, rowHeight);
        painter.drawRect(col5, y, 300, rowHeight);
        painter.drawRect(col6, y, 620, rowHeight);

        // Dessiner le contenu des cellules
        painter.setFont(textFont);
        painter.setPen(Qt::black);

        QRect idRect(col1, y, 150, rowHeight);
        painter.drawText(idRect, Qt::AlignCenter, query.value(0).toString());

        QRect nomRect(col2, y, 470, rowHeight);
        painter.drawText(nomRect, Qt::AlignCenter, query.value(1).toString());

        QRect prenomRect(col3, y, 530, rowHeight);
        painter.drawText(prenomRect, Qt::AlignCenter, query.value(2).toString());

        QRect telRect(col4, y, 300, rowHeight);
        painter.drawText(telRect, Qt::AlignCenter, query.value(3).toString());

        QRect adressRect(col5, y, 300, rowHeight);
        painter.drawText(adressRect, Qt::AlignCenter, query.value(4).toString());

        QRect catRect(col6, y, 620, rowHeight);
        painter.drawText(catRect, Qt::AlignCenter, query.value(5).toString());

        y += rowHeight;
    }

    painter.end();
    return true;
}
//statistiques
QSqlQueryModel* Fournisseur::statistiquesParCategorie()
{
    QSqlQueryModel *model = new QSqlQueryModel();
    model->setQuery(
        "SELECT REGEXP_REPLACE(UPPER(CATEGORIE_FOURNISSEUR), '[^A-Z& ]', '') AS categorie, "
        "COUNT(*) AS total "
        "FROM FOURNISSEURS "
        "GROUP BY REGEXP_REPLACE(UPPER(CATEGORIE_FOURNISSEUR), '[^A-Z& ]', '')"
        );
    return model;
}

//email
bool Fournisseur::envoyerEmailAlerte(QString email, QString nom, QString message)
{
    QNetworkAccessManager *manager = new QNetworkAccessManager();

    // URL API Brevo
    QUrl url("https://api.brevo.com/v3/smtp/email");
    QNetworkRequest request(url);

    // Ajout headers obligatoires
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("api-key", "");

    // Construire le JSON
    QJsonObject emailData;
    emailData["sender"] = QJsonObject{
        {"name", "OpticalStore System"},
        {"email", "opticalstore.alerts@gmail.com"}
    };

    QJsonObject toRecipient;
    toRecipient["email"] = email;
    toRecipient["name"] = nom;
    QJsonArray toList;
    toList.append(toRecipient);

    emailData["to"] = toList;
    emailData["subject"] = QString("Alerte : Anomalie détectée");
    emailData["htmlContent"] = "<p>" + message + "</p>";

    QJsonDocument doc(emailData);
    QByteArray data = doc.toJson();

    // Envoi HTTP POST
    QNetworkReply *reply = manager->post(request, data);

    // Vérification résultat
    QObject::connect(reply, &QNetworkReply::finished, [reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "Email envoyé avec succès !";
        } else {
            qDebug() << "Erreur envoi email :" << reply->errorString();
        }
        reply->deleteLater();
    });

    return true;
}
