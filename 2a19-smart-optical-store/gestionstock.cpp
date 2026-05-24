#include "gestionstock.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QTableWidgetItem>
#include <QPrinter>
#include <QTextDocument>
#include <QTextTable>
#include <QFileDialog>
#include <QTextCursor>
#include <QComboBox>
#include <QSqlRecord>
#include <QSet>
#include <QBrush>
#include <QColor>
#include <cmath>

// NOTE : Version plus robuste de gestionstock.cpp
// - Ajout de vérifications nullptr pour QTableWidget / QComboBox
// - Utilisation de QSqlRecord pour récupérer les valeurs par nom de colonne
// - Protection avant d'utiliser table->item(...)
// - Logs supplémentaires pour aider au debug

bool gestionstock::ajouterProduit(const QString &nom, int quantite, const QString &marque,
                                  const QString &categorie, double prix, int id_fournisseur)
{
    QSqlQuery query;
    query.prepare("INSERT INTO PRODUIT (ID_PRODUIT, NOM_PRODUIT, QUANTITE, MARQUE, CATEGORIE, PRIX_UNITAIRE, ID_FOURNISSEUR) "
                  "VALUES (seq_produit.NEXTVAL, :nom, :quantite, :marque, :categorie, :prix, :idf)");
    query.bindValue(":nom", nom.trimmed());
    query.bindValue(":quantite", quantite);
    query.bindValue(":marque", marque.trimmed());
    query.bindValue(":categorie", categorie.trimmed());
    query.bindValue(":prix", prix);
    query.bindValue(":idf", id_fournisseur);

    if (!query.exec()) {
        qDebug() << "Erreur insertion produit:" << query.lastError().text();
        return false;
    }
    return true;
}

bool gestionstock::modifierProduit(int id, const QString &nom, int quantite, const QString &marque,
                                   const QString &categorie, double prix, int id_fournisseur)
{
    QSqlQuery query;
    query.prepare("UPDATE PRODUIT SET NOM_PRODUIT=:nom, QUANTITE=:quantite, MARQUE=:marque, "
                  "CATEGORIE=:categorie, PRIX_UNITAIRE=:prix, ID_FOURNISSEUR=:idf "
                  "WHERE ID_PRODUIT=:id");
    query.bindValue(":nom", nom.trimmed());
    query.bindValue(":quantite", quantite);
    query.bindValue(":marque", marque.trimmed());
    query.bindValue(":categorie", categorie.trimmed());
    query.bindValue(":prix", prix);
    query.bindValue(":idf", id_fournisseur);
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Erreur modification produit:" << query.lastError().text();
        return false;
    }
    return true;
}

bool gestionstock::supprimerProduit(int id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM PRODUIT WHERE ID_PRODUIT=:id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Erreur suppression produit:" << query.lastError().text();
        return false;
    }
    return true;
}

void gestionstock::rechercherProduit(QTableWidget *table, const QString &nom) const
{
    if (!table) { qDebug() << "rechercherProduit: table null"; return; }

    table->clear();
    table->setRowCount(0);
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({"ID_Produit", "Nom_Produit", "Catégorie", "Marque_Produit", "Quantité_dispo", "Prix_Unitaire"});

    QSqlQuery query;
    query.prepare("SELECT ID_PRODUIT, NOM_PRODUIT, CATEGORIE, MARQUE, QUANTITE, PRIX_UNITAIRE "
                  "FROM PRODUIT WHERE LOWER(NOM_PRODUIT) LIKE LOWER(:nom) ORDER BY ID_PRODUIT ASC");
    query.bindValue(":nom", "%" + nom.trimmed() + "%");

    if (!query.exec()) {
        qDebug() << "Erreur recherche produit:" << query.lastError().text();
        return;
    }

    int row = 0;
    while (query.next()) {
        QSqlRecord rec = query.record();
        table->insertRow(row);
        for (int col = 0; col < 6; ++col) {
            QString text = rec.value(col).toString();
            QTableWidgetItem *item = new QTableWidgetItem(text);
            if (col == 4 || col == 5) item->setData(Qt::EditRole, rec.value(col));
            table->setItem(row, col, item);
            if (table->item(row, col)) table->item(row, col)->setForeground(Qt::black);
        }
        row++;
    }
}

void gestionstock::rechercherParNomOuId(QTableWidget *table, const QString &texte) const
{
    if (!table) { qDebug() << "rechercherParNomOuId: table null"; return; }

    table->clear();
    table->setRowCount(0);
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({"ID_Produit", "Nom_Produit", "Catégorie", "Marque_Produit", "Quantité_dispo", "Prix_Unitaire"});

    QSqlQuery query;
    query.prepare("SELECT ID_PRODUIT, NOM_PRODUIT, CATEGORIE, MARQUE, QUANTITE, PRIX_UNITAIRE "
                  "FROM PRODUIT "
                  "WHERE LOWER(NOM_PRODUIT) LIKE LOWER(:texte) OR TO_CHAR(ID_PRODUIT) LIKE :texte "
                  "ORDER BY ID_PRODUIT ASC");
    query.bindValue(":texte", "%" + texte.trimmed() + "%");

    if (!query.exec()) {
        qDebug() << "Erreur recherche produit:" << query.lastError().text();
        return;
    }

    int row = 0;
    while (query.next()) {
        QSqlRecord rec = query.record();
        table->insertRow(row);
        for (int col = 0; col < 6; ++col) {
            QString text = rec.value(col).toString();
            QTableWidgetItem *item = new QTableWidgetItem(text);
            if (col == 4 || col == 5) item->setData(Qt::EditRole, rec.value(col));
            table->setItem(row, col, item);
            if (table->item(row, col)) table->item(row, col)->setForeground(Qt::black);
        }
        row++;
    }
}

void gestionstock::trierProduits(QTableWidget *table, const QString &critere, bool ordreCroissant) const
{
    if (!table) { qDebug() << "trierProduits: table null"; return; }

    // Déterminer la colonne SQL à trier
    QString colonne;
    if (critere == "Nom")
        colonne = "NOM_PRODUIT";
    else if (critere == "Catégorie")
        colonne = "CATEGORIE";
    else if (critere == "Prix")
        colonne = "PRIX_UNITAIRE";
    else if (critere == "Quantité")
        colonne = "QUANTITE";
    else
        colonne = "ID_PRODUIT";

    QString order = ordreCroissant ? "ASC" : "DESC";

    // Construire la requête SQL pour trier correctement les nombres
    QString queryStr;
    if (colonne == "QUANTITE" || colonne == "PRIX_UNITAIRE") {
        // CAST pour forcer le tri numérique
        queryStr = QString(
                       "SELECT ID_PRODUIT, NOM_PRODUIT, CATEGORIE, MARQUE, QUANTITE, PRIX_UNITAIRE "
                       "FROM PRODUIT ORDER BY TO_NUMBER(%1) %2").arg(colonne, order);
    } else {
        queryStr = QString(
                       "SELECT ID_PRODUIT, NOM_PRODUIT, CATEGORIE, MARQUE, QUANTITE, PRIX_UNITAIRE "
                       "FROM PRODUIT ORDER BY %1 %2").arg(colonne, order);
    }

    QSqlQuery query(queryStr);
    if (!query.exec()) {
        qDebug() << "Erreur SQL tri:" << query.lastError().text();
        return;
    }

    // Vider le tableau et remplir avec le résultat trié
    table->clear();
    table->setRowCount(0);
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({"ID_Produit", "Nom_Produit", "Catégorie", "Marque_Produit", "Quantité_dispo", "Prix_Unitaire"});

    int row = 0;
    while (query.next()) {
        QSqlRecord rec = query.record();
        table->insertRow(row);
        for (int col = 0; col < 6; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(rec.value(col).toString());
            table->setItem(row, col, item);
            if (table->item(row, col)) table->item(row, col)->setForeground(Qt::black);
        }
        row++;
    }
}

double gestionstock::rotationJournaliere(int idProduit, int jours)
{
    if (jours <= 0) jours = 7;

    QSqlQuery query;
    query.prepare(R"(
        SELECT NVL(SUM(QUANTITE_VENDUE), 0)
        FROM VENTES
        WHERE ID_PRODUIT = :id AND DATE_VENTE >= SYSDATE - :jours
    )");
    query.bindValue(":id", idProduit);
    query.bindValue(":jours", jours);

    if (!query.exec()) {
        qDebug() << "Erreur rotationJournaliere:" << query.lastError().text();
        return 0.0;
    }

    if (query.next()) {
        double totalVendu = query.value(0).toDouble();
        double rotation = totalVendu / static_cast<double>(jours);
        qDebug() << "Produit" << idProduit << "rotation journalière:" << rotation;
        return rotation;
    }

    return 0.0;
}

void gestionstock::mettreAJourStockSecurite(QTableWidget *table)
{
    if (!table) {
        qDebug() << "mettreAJourStockSecurite: table null";
        return;
    }

    // === Colonnes ===
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({
        "ID Produit",
        "Nom Produit",
        "Quantité Stock",
        "Stock Sécurité",
        "Total Vendu",
        "Stock Réel"
    });

    table->setRowCount(0);

    // === Récupérer TOUS les produits + ventes éventuelles ===
    QString sql = R"(
        SELECT
            P.ID_PRODUIT,
            P.NOM_PRODUIT,
            P.QUANTITE,
            NVL(SUM(V.QUANTITE_VENDU), 0) AS TOTAL_VENDU
        FROM PRODUIT P
        LEFT JOIN VENTES V ON P.ID_PRODUIT = V.ID_PRODUIT
        GROUP BY
            P.ID_PRODUIT,
            P.NOM_PRODUIT,
            P.QUANTITE
        ORDER BY P.ID_PRODUIT
    )";

    QSqlQuery query;
    if (!query.exec(sql)) {
        qDebug() << "Erreur SQL stock sécurité :" << query.lastError().text();
        return;
    }

    int row = 0;
    while (query.next()) {
        int idProduit = query.value("ID_PRODUIT").toInt();
        QString nomProduit = query.value("NOM_PRODUIT").toString();
        int quantite = query.value("QUANTITE").toInt();
        int totalVendu = query.value("TOTAL_VENDU").toInt();

        // Rotation journalière (30 jours)
        double rotation = rotationJournaliere(idProduit, 30);

        // Stock sécurité (7 jours * 1.5)
        int stockSecurite = std::max(1, static_cast<int>(std::ceil(rotation * 7.0 * 1.5)));

        // Stock réel
        int stockReel = quantite - totalVendu;

        // Ajouter la ligne
        table->insertRow(row);

        table->setItem(row, 0, new QTableWidgetItem(QString::number(idProduit)));
        table->setItem(row, 1, new QTableWidgetItem(nomProduit));
        table->setItem(row, 2, new QTableWidgetItem(QString::number(quantite)));
        table->setItem(row, 3, new QTableWidgetItem(QString::number(stockSecurite)));
        table->setItem(row, 4, new QTableWidgetItem(QString::number(totalVendu)));
        table->setItem(row, 5, new QTableWidgetItem(QString::number(stockReel)));

        // Couleur rouge si stock < stock sécurité
        QColor couleur = (stockReel <= stockSecurite ? Qt::red : Qt::black);
        for (int col = 0; col < 6; ++col)
            table->item(row, col)->setForeground(couleur);

        row++;
    }
}



void gestionstock::afficherProduits(QTableWidget* table)
{
    if (!table) { qDebug() << "afficherProduits: table null"; return; }

    table->clearContents();
    table->setRowCount(0);
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({"ID_Produit", "Nom_Produit", "Catégorie", "Marque", "Quantité_dispo", "Prix_Unitaire"});

    QSqlQuery query("SELECT ID_PRODUIT, NOM_PRODUIT, CATEGORIE, MARQUE, QUANTITE, PRIX_UNITAIRE FROM PRODUIT ORDER BY ID_PRODUIT ASC");
    int row = 0;
    while (query.next()) {
        QSqlRecord rec = query.record();
        table->insertRow(row);
        for (int col = 0; col < 6; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(rec.value(col).toString());
            table->setItem(row, col, item);
            if (table->item(row, col)) table->item(row, col)->setForeground(Qt::black);
        }
        row++;
    }
}

void gestionstock::exporterEnPDF(QTableWidget* table, const QString &nomFichier)
{
    if (!table) { qDebug() << "exporterEnPDF: table null"; return; }

    if (table->rowCount() == 0) {
        qDebug() << "Le tableau est vide, rien à exporter.";
        return;
    }

    QString fichier = nomFichier;
    if (fichier.isEmpty()) {
        fichier = QFileDialog::getSaveFileName(nullptr, "Exporter en PDF", "", "*.pdf");
        if (fichier.isEmpty())
            return;
    }

    if (!fichier.endsWith(".pdf"))
        fichier += ".pdf";

    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fichier);

    QTextDocument doc;
    QTextCursor cursor(&doc);

    cursor.insertHtml("<h2>Liste des produits</h2><br>");

    QTextTableFormat tableFormat;
    tableFormat.setBorder(1);
    tableFormat.setCellPadding(4);
    tableFormat.setAlignment(Qt::AlignCenter);

    QTextTable *textTable = cursor.insertTable(table->rowCount() + 1, table->columnCount(), tableFormat);

    for (int col = 0; col < table->columnCount(); ++col) {
        QString header = table->horizontalHeaderItem(col) ? table->horizontalHeaderItem(col)->text() : QString();
        textTable->cellAt(0, col).firstCursorPosition().insertText(header);
    }

    for (int r = 0; r < table->rowCount(); ++r) {
        for (int c = 0; c < table->columnCount(); ++c) {
            QString cellText = table->item(r, c) ? table->item(r, c)->text() : QString();
            textTable->cellAt(r + 1, c).firstCursorPosition().insertText(cellText);
        }
    }

    doc.print(&printer);
    qDebug() << "Exportation PDF terminée : " << fichier;
}

void gestionstock::remplirComboVentes(QComboBox* comboBox)
{
    if (!comboBox) { qDebug() << "remplirComboVentes: comboBox null"; return; }

    comboBox->clear();

    QSqlQuery query;
    query.prepare("SELECT ID_VENTE FROM VENTES ORDER BY ID_VENTE ASC");

    if (query.exec()) {
        while (query.next()) {
            int id_vente = query.value(0).toInt();
            comboBox->addItem(QString::number(id_vente));
        }
    } else {
        qDebug() << "Erreur lors du chargement des ventes:" << query.lastError().text();
    }
}

void gestionstock::remplirComboProduits(QComboBox* comboBox)
{
    if (!comboBox) { qDebug() << "remplirComboProduits: comboBox null"; return; }

    comboBox->clear();

    QSqlQuery query;
    query.prepare("SELECT ID_PRODUIT, NOM_PRODUIT FROM PRODUIT ORDER BY NOM_PRODUIT ASC");

    if (query.exec()) {
        while (query.next()) {
            int id_produit = query.value(0).toInt();
            QString nom_produit = query.value(1).toString();
            comboBox->addItem(nom_produit, id_produit);
        }
    } else {
        qDebug() << "Erreur lors du chargement des produits:" << query.lastError().text();
    }
}

bool gestionstock::recupererPrixProduit(int id_produit, double &prix)
{
    QSqlQuery query;
    query.prepare("SELECT PRIX_UNITAIRE FROM PRODUIT WHERE ID_PRODUIT = :id_produit");
    query.bindValue(":id_produit", id_produit);

    if (!query.exec()) {
        qDebug() << "Erreur lors de la récupération du prix:" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        prix = query.value(0).toDouble();
        return true;
    }

    return false;
}

bool gestionstock::produitExiste(int id_produit)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM PRODUIT WHERE ID_PRODUIT = :id");
    query.bindValue(":id", id_produit);

    if (!query.exec() || !query.next()) {
        return false;
    }

    return query.value(0).toInt() > 0;
}

bool gestionstock::recupererRemiseVente(int id_vente, double &remise)
{
    QSqlQuery query;
    query.prepare("SELECT REMISE_VENTE FROM VENTES WHERE ID_VENTE = :id_vente");
    query.bindValue(":id_vente", id_vente);

    if (!query.exec()) {
        qDebug() << "Erreur lors de la récupération de la remise:" << query.lastError().text();
        return false;
    }

    if (query.next()) {
        QVariant value = query.value(0);
        if (value.isNull()) {
            remise = 0.0;
        } else {
            remise = value.toDouble();
        }
        qDebug() << "Remise trouvée pour vente" << id_vente << ":" << remise;
        return true;
    }

    qDebug() << "Aucune remise trouvée pour vente ID:" << id_vente;
    remise = 0.0;
    return false;
}
// Remplit le combo des ventes avec une option par défaut
void gestionstock::remplirComboVentesAvecParDefaut(QComboBox* comboBox) {
    if (!comboBox) return;
    comboBox->clear();
    comboBox->addItem("Sélectionnez une vente"); // Valeur par défaut
    remplirComboVentes(comboBox);                 // Remplit avec les ventes existantes
}

// Remplit le combo des produits avec une option par défaut
void gestionstock::remplirComboProduitsAvecParDefaut(QComboBox* comboBox) {
    if (!comboBox) return;
    comboBox->clear();
    comboBox->addItem("Sélectionnez un produit"); // Valeur par défaut
    remplirComboProduits(comboBox);               // Remplit avec les produits existants
}

