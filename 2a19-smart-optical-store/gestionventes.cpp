#include "gestionventes.h"
#include <QSqlError>
#include <QDebug>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QTextDocument>
#include <QPdfWriter>
#include <QPageSize>
#include <QPageLayout>
#include <QPainter>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QNetworkReply>
#include <QObject>

GestionVentes::GestionVentes()
{
    networkManager = new QNetworkAccessManager();
}

GestionVentes::GestionVentes(int id, QString date, double ht, double t, double r, double ttc, int idC, int idE)
{
    id_vente = id;
    date_vente = date;
    prix_ht = ht;
    tva = t;
    remise = r;
    prix_ttc = ttc;
    id_client = idC;
    id_employe = idE;
    networkManager = new QNetworkAccessManager();
}

GestionVentes::~GestionVentes()
{
    delete networkManager;
}

QString GestionVentes::getNumeroResponsable()
{
    QSqlQuery query;

    query.prepare("SELECT TELEPHONE_EMPLOYE FROM EMPLOYES WHERE UPPER(POSTE_EMPLOYE) = 'RESPONSABLE VENTE'");

    if (query.exec() && query.next()) {
        QString telephone = QString::number(query.value(0).toLongLong());
        qDebug() << "📱 Responsable Ventes trouvé:" << telephone;
        return "+216" + telephone;
    }

    //AUCUN RESPONSABLE VENTES TROUVÉ
    qDebug() << "❌ ERREUR: Aucun responsable vente trouvé dans la base";
    return "pas de responsable vente";
}

void GestionVentes::envoyerSMS(const QString &message) {
    QString accountSid = "";
    QString authToken = "";
    QString fromPhone = "";

    QString toPhone = getNumeroResponsable();

    if (toPhone == "pas de responsable vente") {
        qDebug() << "❌ IMPOSSIBLE d'envoyer SMS: pas de responsable vente";
        QMessageBox::warning(nullptr, "Envoi SMS impossible",
                             "Aucun responsable vente trouvé pour l'envoi de SMS.");
        return; // Arrêter l'envoi
    }

    qDebug() << "=== DÉBOGAGE SMS ===";
    qDebug() << "📞 From (Twilio):" << fromPhone;
    qDebug() << "📞 To (Responsable):" << toPhone;
    qDebug() << "💬 Message:" << message;

    QString url = QString("https://api.twilio.com/2010-04-01/Accounts/%1/Messages.json").arg(accountSid);

    QUrl twilioUrl(url);
    QNetworkRequest request;
    request.setUrl(twilioUrl);

    QString auth = accountSid + ":" + authToken;
    request.setRawHeader("Authorization", "Basic " + QByteArray(auth.toUtf8()).toBase64());
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery params;
    params.addQueryItem("To", toPhone);
    params.addQueryItem("From", fromPhone);
    params.addQueryItem("Body", message);

    qDebug() << "🔄 Envoi du SMS...";

    QNetworkReply *reply = networkManager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());

    QObject::connect(reply, &QNetworkReply::finished, [reply]() {
        if (reply->error() == QNetworkReply::NoError) {
            qDebug() << "✅ SMS envoyé avec succès!";
        } else {
            qDebug() << "❌ Erreur SMS:" << reply->errorString();
            qDebug() << "Détails:" << reply->readAll();
        }
        reply->deleteLater();
    });
}

void GestionVentes::notifierVente(const QString &action, int idVente, const QString &date,
                                  const QString &categorie, int quantite, double remise, double prixTTC) {
    QString dateFormatee = date;

    if (date.contains("T")) {
        dateFormatee = date.left(10);
        dateFormatee = dateFormatee.replace("-", "/");
        QStringList parties = dateFormatee.split("/");
        if (parties.size() == 3) {
            dateFormatee = parties[2] + "/" + parties[1] + "/" + parties[0];
        }
    }

    // 📱 FORMATAGE DIRECT (plus de requête SQL)
    QString message = QString("🔔 %1 VENTE\n"
                              "ID: %2\n"
                              "Date: %3\n"
                              "Catégorie: %4\n"
                              "Quantité: %5\n"
                              "Remise: %6%\n"
                              "Total: %7€")
                          .arg(action.toUpper())
                          .arg(idVente)
                          .arg(dateFormatee)
                          .arg(categorie)
                          .arg(quantite)
                          .arg(remise)
                          .arg(QString::number(prixTTC, 'f', 2));

    envoyerSMS(message);
}

bool GestionVentes::ajouter()
{
    if (id_vente <= 0) {
        qDebug() << "❌ ID vente invalide:" << id_vente;
        return false;
    }
    if (prix_ht < 0 || tva < 0 || remise < 0 || prix_ttc < 0) {
        qDebug() << "❌ Valeurs négatives non autorisées";
        return false;
    }

    QSqlQuery query;
    query.prepare("INSERT INTO VENTES "
                  "(ID_VENTE, DATE_VENTE, PRIX_HT_VENTE, TVA_VENTE, REMISE_VENTE, "
                  "PRIX_TTC_VENTE, ID_CLIENT, ID_EMPLOYE) "
                  "VALUES (:id_vente, TO_DATE(:date_vente, 'DD/MM/YYYY'), :prix_ht, :tva, :remise, :prix_ttc, :id_client, :id_employe)");

    query.bindValue(":id_vente", id_vente);
    query.bindValue(":date_vente", date_vente);
    query.bindValue(":prix_ht", prix_ht);
    query.bindValue(":tva", tva);
    query.bindValue(":remise", remise);
    query.bindValue(":prix_ttc", prix_ttc);
    query.bindValue(":id_client", id_client);
    query.bindValue(":id_employe", id_employe);

    if (!query.exec()) {
        qDebug() << "❌ ERREUR AJOUT VENTE:" << query.lastError().text();
        return false;
    }

    qDebug() << "✅ Vente ajoutée avec succès - ID:" << id_vente;

    // 🔍 MAINTENANT RÉCUPÉRER LES DÉTAILS (après création de la vente)
    QString categorie = "Aucun détail";
    int quantiteTotale = 0;

    QSqlQuery queryDetails;
    queryDetails.prepare("SELECT p.CATEGORIE, SUM(c.QUANTITE_DETAIL) as QUANTITE_TOTALE "
                         "FROM CONTENIR c "
                         "JOIN PRODUIT p ON c.ID_PRODUIT = p.ID_PRODUIT "
                         "WHERE c.ID_VENTE = :id_vente "
                         "GROUP BY p.CATEGORIE");
    queryDetails.bindValue(":id_vente", id_vente);

    if (queryDetails.exec() && queryDetails.next()) {
        categorie = queryDetails.value(0).toString();
        quantiteTotale = queryDetails.value(1).toInt();

        if (queryDetails.next()) {
            categorie += " (+ autres)";
        }
    } else {
        qDebug() << "ℹ️ Aucun détail trouvé pour la vente ID:" << id_vente;
    }

    // 📱 Notification SMS après ajout avec toutes les infos
    notifierVente("AJOUT", id_vente, date_vente, categorie, quantiteTotale, remise, prix_ttc);

    return true;
}

bool GestionVentes::supprimer(int id)
{
    if (id <= 0) {
        qDebug() << "❌ ID invalide pour suppression:" << id;
        return false;
    }

    // Récupérer les informations de la vente AVANT suppression
    QSqlQuery selectQuery;
    selectQuery.prepare("SELECT DATE_VENTE, REMISE_VENTE, PRIX_TTC_VENTE FROM VENTES WHERE ID_VENTE = :id_vente");
    selectQuery.bindValue(":id_vente", id);

    QString dateVente;
    double remiseVente = 0;
    double prixTTCVente = 0;

    if (selectQuery.exec() && selectQuery.next()) {
        dateVente = selectQuery.value(0).toString();
        remiseVente = selectQuery.value(1).toDouble();
        prixTTCVente = selectQuery.value(2).toDouble();
    } else {
        qDebug() << "❌ Vente non trouvée avec ID:" << id;
        return false;
    }

    // 🔍 RÉCUPÉRER CATÉGORIE ET QUANTITÉ AVANT SUPPRESSION
    QString categorie = "Aucun détail";
    int quantiteTotale = 0;

    QSqlQuery queryDetails;
    queryDetails.prepare("SELECT p.CATEGORIE, SUM(c.QUANTITE_DETAIL) as QUANTITE_TOTALE "
                         "FROM CONTENIR c "
                         "JOIN PRODUIT p ON c.ID_PRODUIT = p.ID_PRODUIT "
                         "WHERE c.ID_VENTE = :id_vente "
                         "GROUP BY p.CATEGORIE");
    queryDetails.bindValue(":id_vente", id);

    if (queryDetails.exec() && queryDetails.next()) {
        categorie = queryDetails.value(0).toString();
        quantiteTotale = queryDetails.value(1).toInt();

        if (queryDetails.next()) {
            categorie += " (+ autres)";
        }
    } else {
        qDebug() << "ℹ️ Aucun détail trouvé pour la vente ID:" << id;
    }

    // MAINTENANT SUPPRIMER LA VENTE
    QSqlQuery query;
    query.prepare("DELETE FROM VENTES WHERE ID_VENTE = :id_vente");
    query.bindValue(":id_vente", id);

    if (!query.exec()) {
        qDebug() << "❌ ERREUR SUPPRESSION:" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        qDebug() << "❌ Aucune vente trouvée avec ID:" << id;
        return false;
    }

    qDebug() << "✅ Vente supprimée avec succès - ID:" << id;

    // 📱 Notification SMS après suppression avec toutes les infos
    notifierVente("SUPPRESSION", id, dateVente, categorie, quantiteTotale, remiseVente, prixTTCVente);

    return true;
}

bool GestionVentes::modifier(int id, QString date, double ht, double t, double r, double ttc, int idC, int idE)
{
    if (id <= 0) {
        qDebug() << "❌ ID vente invalide:" << id;
        return false;
    }
    if (ht < 0 || t < 0 || r < 0 || ttc < 0) {
        qDebug() << "❌ Valeurs négatives non autorisées";
        return false;
    }

    // Vérifier d'abord si la vente existe
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM VENTES WHERE ID_VENTE = :id_vente");
    checkQuery.bindValue(":id_vente", id);

    if (!checkQuery.exec() || !checkQuery.next() || checkQuery.value(0).toInt() == 0) {
        qDebug() << "❌ Aucune vente trouvée avec ID:" << id;
        return false;
    }

    // 🔍 RÉCUPÉRER CATÉGORIE ET QUANTITÉ AVANT MODIFICATION
    QString categorie = "Aucun détail";
    int quantiteTotale = 0;

    QSqlQuery queryDetails;
    queryDetails.prepare("SELECT p.CATEGORIE, SUM(c.QUANTITE_DETAIL) as QUANTITE_TOTALE "
                         "FROM CONTENIR c "
                         "JOIN PRODUIT p ON c.ID_PRODUIT = p.ID_PRODUIT "
                         "WHERE c.ID_VENTE = :id_vente "
                         "GROUP BY p.CATEGORIE");
    queryDetails.bindValue(":id_vente", id);

    if (queryDetails.exec() && queryDetails.next()) {
        categorie = queryDetails.value(0).toString();
        quantiteTotale = queryDetails.value(1).toInt();

        if (queryDetails.next()) {
            categorie += " (+ autres)";
        }
    } else {
        qDebug() << "ℹ️ Aucun détail trouvé pour la vente ID:" << id;
    }

    // MAINTENANT MODIFIER LA VENTE
    QSqlQuery query;
    query.prepare("UPDATE VENTES SET "
                  "DATE_VENTE = TO_DATE(:date_vente, 'DD/MM/YYYY'), "
                  "PRIX_HT_VENTE = :prix_ht, "
                  "TVA_VENTE = :tva, "
                  "REMISE_VENTE = :remise, "
                  "PRIX_TTC_VENTE = :prix_ttc, "
                  "ID_CLIENT = :id_client, "
                  "ID_EMPLOYE = :id_employe "
                  "WHERE ID_VENTE = :id_vente");

    query.bindValue(":id_vente", id);
    query.bindValue(":date_vente", date);
    query.bindValue(":prix_ht", ht);
    query.bindValue(":tva", t);
    query.bindValue(":remise", r);
    query.bindValue(":prix_ttc", ttc);
    query.bindValue(":id_client", idC);
    query.bindValue(":id_employe", idE);

    if (!query.exec()) {
        qDebug() << "❌ ERREUR MODIFICATION:" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        qDebug() << "❌ Aucune vente trouvée avec ID:" << id;
        return false;
    }

    qDebug() << "✅ Vente modifiée avec succès - ID:" << id;

    // 📱 Notification SMS après modification avec toutes les infos
    notifierVente("MODIFICATION", id, date, categorie, quantiteTotale, r, ttc);

    return true;
}

QSqlQueryModel* GestionVentes::afficher()
{
    QSqlQueryModel* model = new QSqlQueryModel();

    // REQUÊTE SÉCURISÉE
    model->setQuery("SELECT ID_VENTE, DATE_VENTE, PRIX_HT_VENTE, TVA_VENTE, REMISE_VENTE, "
                    "PRIX_TTC_VENTE, ID_CLIENT, ID_EMPLOYE FROM VENTES");

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID Vente"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Date Vente"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Prix HT"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("%TVA"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("%Remise"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Prix TTC"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("ID Client"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("ID Employé"));

    return model;
}

bool GestionVentes::exporterPDF(const QString& cheminFichier)
{
    try {
        // Récupérer les données des ventes
        QSqlQuery query;
        if (!query.exec("SELECT ID_VENTE, DATE_VENTE, PRIX_HT_VENTE, TVA_VENTE, REMISE_VENTE, "
                        "PRIX_TTC_VENTE, ID_CLIENT, ID_EMPLOYE FROM VENTES ORDER BY ID_VENTE")) {
            qDebug() << "❌ Erreur lors de la récupération des ventes:" << query.lastError().text();
            return false;
        }

        int nbVentes = 0;
        double totalHT = 0;
        double totalTTC = 0;
        double totalTVA = 0;

        // Préparer le HTML avec une mise en page adaptée au PDF
        QString html;
        html += "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
        html += "<style>";
        html += "body { font-family: Arial, sans-serif; margin: 30px; background: white; color: black; }";
        html += "h1 { color: #2c3e50; text-align: center; margin-bottom: 20px; font-size: 70px; font-weight: bold; }";
        html += ".header { text-align: center; margin-bottom: 40px; }";
        html += ".info { text-align: center; color: #7f8c8d; margin: 10px 0; font-size: 35px; }";
        html += ".subtitle { color: #3498db; font-weight: bold; font-size: 60px; margin: 40px 0 25px 0; text-align: center; }";
        html += "table { width: 98%; border-collapse: collapse; margin: 30px auto; font-size: 55px; }";
        html += "th { background: #3498db; color: white; padding: 25px 20px; text-align: center; font-weight: bold; border: 3px solid #2980b9; font-size: 45px; }";
        html += "td { padding: 20px 15px; border: 2px solid #ddd; text-align: center; font-size: 40px; }";
        html += "tr:nth-child(even) { background: #f8f9fa; }";
        html += ".stats-table { width: 90%; margin: 40px auto; background: #f8f9fa; border: 3px solid #ddd; padding: 30px; border-radius: 15px; }";
        html += ".stats-row { display: flex; justify-content: space-between; padding: 20px 0; border-bottom: 2px solid #e0e0e0; font-size: 40px; }";
        html += ".stats-label { font-weight: 600; color: #2c3e50; }";
        html += ".stats-value { color: #27ae60; font-weight: bold; }";
        html += ".footer { margin-top: 50px; text-align: center; color: #95a5a6; font-size: 30px; padding-top: 20px; border-top: 3px solid #ecf0f1; }";
        html += ".no-data { text-align: center; color: #e74c3c; font-size: 50px; margin: 60px 0; font-weight: bold; }";
        html += ".table-container { width: 100%; overflow-x: auto; margin: 0 auto; }";
        html += "</style>";
        html += "</head><body>";

        // En-tête du document
        html += "<div class='header'>";
        html += "<h1>RAPPORT DES VENTES</h1>";
        html += "<div class='info'>OPTICAL STORE</div>";
        html += "<div class='info'>" + QDateTime::currentDateTime().toString("dd/MM/yyyy à HH:mm") + "</div>";
        html += "</div>";

        // Construction du tableau des ventes
        html += "<div class='subtitle'>DÉTAIL DES VENTES</div>";
        html += "<div class='table-container'>";
        html += "<table>";
        html += "<thead><tr>";
        html += "<th>ID</th>";
        html += "<th>DATE</th>";
        html += "<th>PRIX HT</th>";
        html += "<th>TVA</th>";
        html += "<th>REMISE</th>";
        html += "<th>PRIX TTC</th>";
        html += "<th>CLIENT</th>";
        html += "<th>EMPLOYÉ</th>";
        html += "</tr></thead><tbody>";

        // Remplissage des données
        while (query.next()) {
            int id = query.value(0).toInt();
            QString date = query.value(1).toString();
            double prixHT = query.value(2).toDouble();
            double tva = query.value(3).toDouble();
            double remise = query.value(4).toDouble();
            double prixTTC = query.value(5).toDouble();
            int idClient = query.value(6).toInt();
            int idEmploye = query.value(7).toInt();

            html += "<tr>";
            html += "<td><strong>" + QString::number(id) + "</strong></td>";
            html += "<td>" + date.left(10) + "</td>";
            html += "<td>" + QString::number(prixHT, 'f', 2) + " €</td>";
            html += "<td>" + QString::number(tva, 'f', 1) + "%</td>";
            html += "<td>" + QString::number(remise, 'f', 1) + "%</td>";
            html += "<td><strong>" + QString::number(prixTTC, 'f', 2) + " €</strong></td>";
            html += "<td>" + QString::number(idClient) + "</td>";
            html += "<td>" + QString::number(idEmploye) + "</td>";
            html += "</tr>";

            nbVentes++;
            totalHT += prixHT;
            totalTTC += prixTTC;
            totalTVA += (prixTTC - prixHT);
        }

        html += "</tbody></table>";
        html += "</div>";

        // Vérifier s'il y a des données
        if (nbVentes == 0) {
            html += "<div class='no-data'>AUCUNE VENTE À AFFICHER</div>";
        } else {
            // Section des statistiques
            html += "<div class='subtitle'>STATISTIQUES</div>";
            html += "<div class='stats-table'>";

            html += "<div class='stats-row'>";
            html += "<span class='stats-label'>Nombre total de ventes :</span>";
            html += "<span class='stats-value'>" + QString::number(nbVentes) + "</span>";
            html += "</div>";

            html += "<div class='stats-row'>";
            html += "<span class='stats-label'>Chiffre d'affaires total HT :</span>";
            html += "<span class='stats-value'>" + QString::number(totalHT, 'f', 2) + " €</span>";
            html += "</div>";

            html += "<div class='stats-row'>";
            html += "<span class='stats-label'>Chiffre d'affaires total TTC :</span>";
            html += "<span class='stats-value'>" + QString::number(totalTTC, 'f', 2) + " €</span>";
            html += "</div>";

            html += "<div class='stats-row'>";
            html += "<span class='stats-label'>Montant total TVA :</span>";
            html += "<span class='stats-value'>" + QString::number(totalTVA, 'f', 2) + " €</span>";
            html += "</div>";

            if (nbVentes > 0) {
                html += "<div class='stats-row'>";
                html += "<span class='stats-label'>Moyenne par vente HT :</span>";
                html += "<span class='stats-value'>" + QString::number(totalHT / nbVentes, 'f', 2) + " €</span>";
                html += "</div>";

                html += "<div class='stats-row'>";
                html += "<span class='stats-label'>Moyenne par vente TTC :</span>";
                html += "<span class='stats-value'>" + QString::number(totalTTC / nbVentes, 'f', 2) + " €</span>";
                html += "</div>";
            }

            html += "</div>";
        }

        // Pied de page
        html += "<div class='footer'>";
        html += "Document généré automatiquement - Optical Store<br>";
        html += "© " + QDateTime::currentDateTime().toString("yyyy") + " - Tous droits réservés";
        html += "</div>";

        html += "</body></html>";

        // === Génération du PDF avec QPdfWriter ===
        QPdfWriter pdf(cheminFichier);
        pdf.setPageSize(QPageSize(QPageSize::A4));
        pdf.setResolution(300);
        pdf.setPageMargins(QMarginsF(15, 15, 15, 15));

        QTextDocument doc;
        doc.setHtml(html);

        // Ajuster la taille du document à la page
        doc.setPageSize(QSizeF(pdf.width(), pdf.height()));

        QPainter painter(&pdf);

        // Rendre le texte noir pour éviter les problèmes de couleur
        painter.setPen(Qt::black);

        // Dessiner le document
        doc.drawContents(&painter);

        painter.end();

        qDebug() << "✅ Exportation PDF réussie:" << nbVentes << "ventes exportées vers" << cheminFichier;
        return (nbVentes > 0);
    }
    catch (...) {
        qDebug() << "❌ Erreur lors de l'exportation PDF";
        return false;
    }
}

QSqlQueryModel* GestionVentes::rechercherParDate(const QString &date)
{
    QSqlQueryModel *model = new QSqlQueryModel();

    QSqlQuery query;
    query.prepare("SELECT * FROM VENTES WHERE DATE_VENTE = :date ORDER BY ID_VENTE");
    query.bindValue(":date", date);

    if (query.exec()) {
        model->setQuery(query);

        // Définir les en-têtes des colonnes
        model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID Vente"));
        model->setHeaderData(1, Qt::Horizontal, QObject::tr("Date Vente"));
        model->setHeaderData(2, Qt::Horizontal, QObject::tr("Prix HT"));
        model->setHeaderData(3, Qt::Horizontal, QObject::tr("TVA"));
        model->setHeaderData(4, Qt::Horizontal, QObject::tr("Remise"));
        model->setHeaderData(5, Qt::Horizontal, QObject::tr("Prix TTC"));
        model->setHeaderData(6, Qt::Horizontal, QObject::tr("ID Client"));
        model->setHeaderData(7, Qt::Horizontal, QObject::tr("ID Employé"));
    } else {
        qDebug() << "❌ Erreur recherche par date:" << query.lastError().text();
        delete model;
        return nullptr;
    }

    return model;
}

// === MÉTHODES POUR LES DÉTAILS DE VENTE (TABLE CONTENIR) ===

bool GestionVentes::ajouterDetailVente(int idVente, int idProduit, int quantite, double prixUnitaire)
{
    if (idVente <= 0 || idProduit <= 0 || quantite <= 0 || prixUnitaire <= 0) {
        qDebug() << "❌ Données invalides pour l'ajout du détail de vente";
        return false;
    }

    double sousTotal = calculerSousTotal(quantite, prixUnitaire);

    QSqlQuery query;
    query.prepare("INSERT INTO CONTENIR "
                  "(ID_VENTE, ID_PRODUIT, QUANTITE_DETAIL, PRIX_UNITAIRE_DETAIL, SOUS_TOTAL_DETAIL) "
                  "VALUES (:id_vente, :id_produit, :quantite, :prix_unitaire, :sous_total)");

    query.bindValue(":id_vente", idVente);
    query.bindValue(":id_produit", idProduit);
    query.bindValue(":quantite", quantite);
    query.bindValue(":prix_unitaire", prixUnitaire);
    query.bindValue(":sous_total", sousTotal);

    if (!query.exec()) {
        qDebug() << "❌ ERREUR AJOUT DÉTAIL VENTE:" << query.lastError().text();
        return false;
    }

    qDebug() << "✅ Détail de vente ajouté avec succès - Vente:" << idVente << "Produit:" << idProduit;
    return true;
}

// NOUVELLE MÉTHODE POUR L'INTERFACE SIMPLIFIÉE
bool GestionVentes::ajouterDetailVenteAvecPrix(int idVente, int idProduit, int quantite)
{
    if (idVente <= 0 || idProduit <= 0 || quantite <= 0) {
        qDebug() << "❌ Données invalides pour l'ajout du détail de vente";
        return false;
    }

    // Récupérer le prix du produit
    QSqlQuery prixQuery;
    prixQuery.prepare("SELECT PRIX_UNITAIRE FROM PRODUIT WHERE ID_PRODUIT = :id_produit");
    prixQuery.bindValue(":id_produit", idProduit);

    double prixUnitaire = 0;
    if (prixQuery.exec() && prixQuery.next()) {
        prixUnitaire = prixQuery.value(0).toDouble();
    } else {
        qDebug() << "❌ Impossible de récupérer le prix du produit";
        return false;
    }

    double sousTotal = calculerSousTotal(quantite, prixUnitaire);

    QSqlQuery query;
    query.prepare("INSERT INTO CONTENIR "
                  "(ID_VENTE, ID_PRODUIT, QUANTITE_DETAIL, PRIX_UNITAIRE_DETAIL, SOUS_TOTAL_DETAIL) "
                  "VALUES (:id_vente, :id_produit, :quantite, :prix_unitaire, :sous_total)");

    query.bindValue(":id_vente", idVente);
    query.bindValue(":id_produit", idProduit);
    query.bindValue(":quantite", quantite);
    query.bindValue(":prix_unitaire", prixUnitaire);
    query.bindValue(":sous_total", sousTotal);

    if (!query.exec()) {
        qDebug() << "❌ ERREUR AJOUT DÉTAIL VENTE:" << query.lastError().text();
        return false;
    }

    qDebug() << "✅ Détail de vente ajouté avec succès - Vente:" << idVente << "Produit:" << idProduit;
    return true;
}

QSqlQueryModel* GestionVentes::afficherDetailsVente()
{
    QSqlQueryModel* model = new QSqlQueryModel();

    // ✅ CORRECTION : FORMATER LA DATE SANS HEURE (jj/mm/aaaa)
    model->setQuery("SELECT c.ID_VENTE, "
                    "TO_CHAR(v.DATE_VENTE, 'DD/MM/YYYY') as DATE_VENTE, "  // ← FORMAT jj/mm/aaaa
                    "c.ID_PRODUIT, p.NOM_PRODUIT, p.CATEGORIE, "
                    "c.QUANTITE_DETAIL, c.PRIX_UNITAIRE_DETAIL, c.SOUS_TOTAL_DETAIL "
                    "FROM CONTENIR c "
                    "JOIN VENTES v ON c.ID_VENTE = v.ID_VENTE "
                    "JOIN PRODUIT p ON c.ID_PRODUIT = p.ID_PRODUIT "
                    "ORDER BY c.ID_VENTE, c.ID_PRODUIT");

    // 🏷️ LABELS FRANÇAIS
    model->setHeaderData(0, Qt::Horizontal, QObject::tr("N° Vente"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Date Vente"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Réf. Produit"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Désignation"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Catégorie"));
    model->setHeaderData(5, Qt::Horizontal, QObject::tr("Qté"));
    model->setHeaderData(6, Qt::Horizontal, QObject::tr("Prix Unitaire (€)"));
    model->setHeaderData(7, Qt::Horizontal, QObject::tr("Montant (€)"));

    return model;
}
bool GestionVentes::supprimerDetailVente(int idVente, int idProduit)
{
    if (idVente <= 0 || idProduit <= 0) {
        qDebug() << "❌ ID vente ou produit invalide pour suppression";
        return false;
    }

    QSqlQuery query;
    query.prepare("DELETE FROM CONTENIR WHERE ID_VENTE = :id_vente AND ID_PRODUIT = :id_produit");
    query.bindValue(":id_vente", idVente);
    query.bindValue(":id_produit", idProduit);

    if (!query.exec()) {
        qDebug() << "❌ ERREUR SUPPRESSION DÉTAIL VENTE:" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        qDebug() << "❌ Aucun détail trouvé pour Vente:" << idVente << "Produit:" << idProduit;
        return false;
    }

    qDebug() << "✅ Détail de vente supprimé avec succès - Vente:" << idVente << "Produit:" << idProduit;
    return true;
}

bool GestionVentes::modifierDetailVente(int idVente, int idProduit, int nouvelleQuantite, double nouveauPrix)
{
    if (idVente <= 0 || idProduit <= 0 || nouvelleQuantite <= 0 || nouveauPrix <= 0) {
        qDebug() << "❌ Données invalides pour la modification du détail";
        return false;
    }

    double nouveauSousTotal = calculerSousTotal(nouvelleQuantite, nouveauPrix);

    QSqlQuery query;
    query.prepare("UPDATE CONTENIR SET "
                  "QUANTITE_DETAIL = :quantite, "
                  "PRIX_UNITAIRE_DETAIL = :prix_unitaire, "
                  "SOUS_TOTAL_DETAIL = :sous_total "
                  "WHERE ID_VENTE = :id_vente AND ID_PRODUIT = :id_produit");

    query.bindValue(":id_vente", idVente);
    query.bindValue(":id_produit", idProduit);
    query.bindValue(":quantite", nouvelleQuantite);
    query.bindValue(":prix_unitaire", nouveauPrix);
    query.bindValue(":sous_total", nouveauSousTotal);

    if (!query.exec()) {
        qDebug() << "❌ ERREUR MODIFICATION DÉTAIL VENTE:" << query.lastError().text();
        return false;
    }

    if (query.numRowsAffected() == 0) {
        qDebug() << "❌ Aucun détail trouvé pour Vente:" << idVente << "Produit:" << idProduit;
        return false;
    }

    qDebug() << "✅ Détail de vente modifié avec succès - Vente:" << idVente << "Produit:" << idProduit;
    return true;
}

QSqlQueryModel* GestionVentes::rechercherDetailsParVente(int idVente)
{
    QSqlQueryModel *model = new QSqlQueryModel();

    QSqlQuery query;
    query.prepare("SELECT c.ID_VENTE, v.DATE_VENTE, "
                  "c.ID_PRODUIT, p.NOM_PRODUIT, p.CATEGORIE, "
                  "c.QUANTITE_DETAIL, c.PRIX_UNITAIRE_DETAIL, c.SOUS_TOTAL_DETAIL "
                  "FROM CONTENIR c "
                  "JOIN VENTES v ON c.ID_VENTE = v.ID_VENTE "
                  "JOIN PRODUIT p ON c.ID_PRODUIT = p.ID_PRODUIT "
                  "WHERE c.ID_VENTE = :id_vente "
                  "ORDER BY c.ID_PRODUIT");
    query.bindValue(":id_vente", idVente);

    if (query.exec()) {
        model->setQuery(query);

        model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID Vente"));
        model->setHeaderData(1, Qt::Horizontal, QObject::tr("Date Vente"));
        model->setHeaderData(2, Qt::Horizontal, QObject::tr("ID Produit"));
        model->setHeaderData(3, Qt::Horizontal, QObject::tr("Nom Produit"));
        model->setHeaderData(4, Qt::Horizontal, QObject::tr("Catégorie"));
        model->setHeaderData(5, Qt::Horizontal, QObject::tr("Quantité"));
        model->setHeaderData(6, Qt::Horizontal, QObject::tr("Prix Unitaire"));
        model->setHeaderData(7, Qt::Horizontal, QObject::tr("Sous-Total"));
    } else {
        qDebug() << "❌ Erreur recherche détails par vente:" << query.lastError().text();
        delete model;
        return nullptr;
    }

    return model;
}

double GestionVentes::calculerSousTotal(int quantite, double prixUnitaire)
{
    return quantite * prixUnitaire;
}

bool GestionVentes::exporterPDFDetails(const QString& cheminFichier)
{
    try {
        // Récupérer les données des détails de vente
        QSqlQuery query;
        if (!query.exec("SELECT c.ID_VENTE, v.DATE_VENTE, "
                        "c.ID_PRODUIT, p.NOM_PRODUIT, p.CATEGORIE, "
                        "c.QUANTITE_DETAIL, c.PRIX_UNITAIRE_DETAIL, c.SOUS_TOTAL_DETAIL "
                        "FROM CONTENIR c "
                        "JOIN VENTES v ON c.ID_VENTE = v.ID_VENTE "
                        "JOIN PRODUIT p ON c.ID_PRODUIT = p.ID_PRODUIT "
                        "ORDER BY c.ID_VENTE, c.ID_PRODUIT")) {
            qDebug() << "❌ Erreur lors de la récupération des détails de vente:" << query.lastError().text();
            return false;
        }

        int nbDetails = 0;
        double totalGeneral = 0;
        QMap<int, double> totalParVente; // Pour les statistiques par vente

        // Préparer le HTML avec une mise en page adaptée au PDF
        QString html;
        html += "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
        html += "<style>";
        html += "body { font-family: Arial, sans-serif; margin: 30px; background: white; color: black; }";
        html += "h1 { color: #2c3e50; text-align: center; margin-bottom: 20px; font-size: 70px; font-weight: bold; }";
        html += ".header { text-align: center; margin-bottom: 40px; }";
        html += ".info { text-align: center; color: #7f8c8d; margin: 10px 0; font-size: 35px; }";
        html += ".subtitle { color: #3498db; font-weight: bold; font-size: 60px; margin: 40px 0 25px 0; text-align: center; }";
        html += "table { width: 98%; border-collapse: collapse; margin: 30px auto; font-size: 35px; }"; // ← 35px ici
        html += "th { background: #3498db; color: white; padding: 25px 20px; text-align: center; font-weight: bold; border: 3px solid #2980b9; font-size: 35px; }"; // ← 35px ici
        html += "td { padding: 20px 15px; border: 2px solid #ddd; text-align: center; font-size: 35px; }"; // ← 35px ici
        html += "tr:nth-child(even) { background: #f8f9fa; }";
        html += ".vente-group { background: #e8f4fd !important; font-weight: bold; }";
        html += ".stats-table { width: 90%; margin: 40px auto; background: #f8f9fa; border: 3px solid #ddd; padding: 30px; border-radius: 15px; }";
        html += ".stats-row { display: flex; justify-content: space-between; padding: 20px 0; border-bottom: 2px solid #e0e0e0; font-size: 40px; }";
        html += ".stats-label { font-weight: 600; color: #2c3e50; }";
        html += ".stats-value { color: #27ae60; font-weight: bold; }";
        html += ".footer { margin-top: 50px; text-align: center; color: #95a5a6; font-size: 30px; padding-top: 20px; border-top: 3px solid #ecf0f1; }";
        html += ".no-data { text-align: center; color: #e74c3c; font-size: 30px; margin: 60px 0; font-weight: bold; }";
        html += ".table-container { width: 100%; overflow-x: auto; margin: 0 auto; }";
        html += "</style>";
        html += "</head><body>";

        // En-tête du document
        html += "<div class='header'>";
        html += "<h1>RAPPORT DES DÉTAILS DE VENTE</h1>";
        html += "<div class='info'>OPTICAL STORE</div>";
        html += "<div class='info'>" + QDateTime::currentDateTime().toString("dd/MM/yyyy à HH:mm") + "</div>";
        html += "</div>";

        // Construction du tableau des détails de vente
        html += "<div class='subtitle'>DÉTAIL DES VENTES PAR PRODUIT</div>";
        html += "<div class='table-container'>";
        html += "<table>";
        html += "<thead><tr>";
        html += "<th>VENTE</th>";
        html += "<th>DATE</th>";
        html += "<th>PRODUIT</th>";
        html += "<th>DÉSIGNATION</th>";
        html += "<th>CATÉGORIE</th>";
        html += "<th>QUANTITÉ</th>";
        html += "<th>PRIX UNITAIRE</th>";
        html += "<th>SOUS-TOTAL</th>";
        html += "</tr></thead><tbody>";
        // Variables pour gérer les groupes de vente
        int currentVente = -1;
        double totalCurrentVente = 0;

        // Remplissage des données
        while (query.next()) {
            int idVente = query.value(0).toInt();
            QString date = query.value(1).toString();
            int idProduit = query.value(2).toInt();
            QString nomProduit = query.value(3).toString();
            QString categorie = query.value(4).toString();
            int quantite = query.value(5).toInt();
            double prixUnitaire = query.value(6).toDouble();
            double sousTotal = query.value(7).toDouble();

            // Nouvelle vente - ajouter une ligne de séparation
            if (idVente != currentVente) {
                if (currentVente != -1) {
                    // Ajouter le total de la vente précédente
                    html += "<tr class='vente-group'>";
                    html += "<td colspan='7' style='text-align: right; font-weight: bold;'>Total Vente " + QString::number(currentVente) + " :</td>";
                    html += "<td style='font-weight: bold;'>" + QString::number(totalCurrentVente, 'f', 2) + " €</td>";
                    html += "</tr>";
                }

                currentVente = idVente;
                totalCurrentVente = 0;
            }

            html += "<tr>";
            html += "<td><strong>" + QString::number(idVente) + "</strong></td>";
            html += "<td>" + date.left(10) + "</td>";
            html += "<td>" + QString::number(idProduit) + "</td>";
            html += "<td>" + nomProduit + "</td>";
            html += "<td>" + categorie + "</td>";
            html += "<td>" + QString::number(quantite) + "</td>";
            html += "<td>" + QString::number(prixUnitaire, 'f', 2) + " €</td>";
            html += "<td><strong>" + QString::number(sousTotal, 'f', 2) + " €</strong></td>";
            html += "</tr>";

            nbDetails++;
            totalGeneral += sousTotal;
            totalCurrentVente += sousTotal;
            totalParVente[idVente] += sousTotal;
        }

        // Ajouter le total de la dernière vente
        if (currentVente != -1) {
            html += "<tr class='vente-group'>";
            html += "<td colspan='7' style='text-align: right; font-weight: bold;'>Total Vente " + QString::number(currentVente) + " :</td>";
            html += "<td style='font-weight: bold;'>" + QString::number(totalCurrentVente, 'f', 2) + " €</td>";
            html += "</tr>";
        }

        html += "</tbody></table>";
        html += "</div>";

        // Vérifier s'il y a des données
        if (nbDetails == 0) {
            html += "<div class='no-data'>AUCUN DÉTAIL DE VENTE À AFFICHER</div>";
        } else {
            // Section des statistiques
            html += "<div class='subtitle'>STATISTIQUES</div>";
            html += "<div class='stats-table'>";

            html += "<div class='stats-row'>";
            html += "<span class='stats-label'>Nombre total de lignes de vente :</span>";
            html += "<span class='stats-value'>" + QString::number(nbDetails) + "</span>";
            html += "</div>";

            html += "<div class='stats-row'>";
            html += "<span class='stats-label'>Nombre total de ventes :</span>";
            html += "<span class='stats-value'>" + QString::number(totalParVente.size()) + "</span>";
            html += "</div>";

            html += "<div class='stats-row'>";
            html += "<span class='stats-label'>Chiffre d'affaires total :</span>";
            html += "<span class='stats-value'>" + QString::number(totalGeneral, 'f', 2) + " €</span>";
            html += "</div>";

            if (nbDetails > 0) {
                html += "<div class='stats-row'>";
                html += "<span class='stats-label'>Moyenne par ligne de vente :</span>";
                html += "<span class='stats-value'>" + QString::number(totalGeneral / nbDetails, 'f', 2) + " €</span>";
                html += "</div>";

                html += "<div class='stats-row'>";
                html += "<span class='stats-label'>Moyenne par vente :</span>";
                html += "<span class='stats-value'>" + QString::number(totalGeneral / totalParVente.size(), 'f', 2) + " €</span>";
                html += "</div>";
            }

            html += "</div>";
        }

        // Pied de page
        html += "<div class='footer'>";
        html += "Document généré automatiquement - Optical Store<br>";
        html += "© " + QDateTime::currentDateTime().toString("yyyy") + " - Tous droits réservés";
        html += "</div>";

        html += "</body></html>";

        // === Génération du PDF avec QPdfWriter ===
        QPdfWriter pdf(cheminFichier);
        pdf.setPageSize(QPageSize(QPageSize::A4));
        pdf.setResolution(300);
        pdf.setPageMargins(QMarginsF(15, 15, 15, 15));

        QTextDocument doc;
        doc.setHtml(html);

        // Ajuster la taille du document à la page
        doc.setPageSize(QSizeF(pdf.width(), pdf.height()));

        QPainter painter(&pdf);

        // Rendre le texte noir pour éviter les problèmes de couleur
        painter.setPen(Qt::black);

        // Dessiner le document
        doc.drawContents(&painter);

        painter.end();

        qDebug() << "✅ Exportation PDF détails réussie:" << nbDetails << "lignes exportées vers" << cheminFichier;
        return (nbDetails > 0);
    }
    catch (...) {
        qDebug() << "❌ Erreur lors de l'exportation PDF détails";
        return false;
    }
}
QSqlQueryModel* GestionVentes::rechercherDetailsParDate(const QString &date)
{
    QSqlQueryModel *model = new QSqlQueryModel();

    QSqlQuery query;
    query.prepare("SELECT c.ID_VENTE, "
                  "TO_CHAR(v.DATE_VENTE, 'DD/MM/YYYY') as DATE_VENTE, "
                  "c.ID_PRODUIT, p.NOM_PRODUIT, p.CATEGORIE, "
                  "c.QUANTITE_DETAIL, c.PRIX_UNITAIRE_DETAIL, c.SOUS_TOTAL_DETAIL "
                  "FROM CONTENIR c "
                  "JOIN VENTES v ON c.ID_VENTE = v.ID_VENTE "
                  "JOIN PRODUIT p ON c.ID_PRODUIT = p.ID_PRODUIT "
                  "WHERE TO_CHAR(v.DATE_VENTE, 'DD/MM/YYYY') = :date_recherche "
                  "ORDER BY c.ID_VENTE, c.ID_PRODUIT");
    query.bindValue(":date_recherche", date);

    if (query.exec()) {
        model->setQuery(query);

        model->setHeaderData(0, Qt::Horizontal, QObject::tr("N° Vente"));
        model->setHeaderData(1, Qt::Horizontal, QObject::tr("Date Vente"));
        model->setHeaderData(2, Qt::Horizontal, QObject::tr("Réf. Produit"));
        model->setHeaderData(3, Qt::Horizontal, QObject::tr("Désignation"));
        model->setHeaderData(4, Qt::Horizontal, QObject::tr("Catégorie"));
        model->setHeaderData(5, Qt::Horizontal, QObject::tr("Qté"));
        model->setHeaderData(6, Qt::Horizontal, QObject::tr("Prix Unitaire (€)"));
        model->setHeaderData(7, Qt::Horizontal, QObject::tr("Montant (€)"));
    } else {
        qDebug() << "❌ Erreur recherche détails par date:" << query.lastError().text();
        delete model;
        return nullptr;
    }

    return model;
}

