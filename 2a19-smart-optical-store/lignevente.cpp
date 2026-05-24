#include "lignevente.h"
#include <QSqlError>
#include <QDebug>
#include <QBrush>

LigneVente::LigneVente()
    : id_ligne(0), id_vente(0), id_produit(0), quantite(0),
    prix_unitaire(0.0), remise_pourcent(0.0)
{
}

LigneVente::LigneVente(int id_ligne, int id_vente, int id_produit,
                       int quantite, double prix_unitaire, double remise_pourcent)
    : id_ligne(id_ligne), id_vente(id_vente),
    id_produit(id_produit), quantite(quantite),
    prix_unitaire(prix_unitaire), remise_pourcent(remise_pourcent)
{
}

double LigneVente::getPrixTotalLigne() const
{
    return quantite * prix_unitaire * (1.0 - remise_pourcent / 100.0);
}

bool LigneVente::verifierStockDisponible(int id_produit, int quantite)
{
    QSqlQuery query;
    query.prepare("SELECT QUANTITE FROM PRODUIT WHERE ID_PRODUIT = :id");
    query.bindValue(":id", id_produit);

    if (query.exec() && query.next()) {
        int stock = query.value(0).toInt();
        return stock >= quantite;
    }
    return false;
}

bool LigneVente::recupererPrixProduit(int id_produit, double &prix)
{
    QSqlQuery query;
    query.prepare("SELECT PRIX_UNITAIRE FROM PRODUIT WHERE ID_PRODUIT = :id");
    query.bindValue(":id", id_produit);

    if (query.exec() && query.next()) {
        prix = query.value(0).toDouble();
        return true;
    }
    return false;
}

QString LigneVente::getNomProduit(int id_produit)
{
    QSqlQuery query;
    query.prepare("SELECT NOM_PRODUIT FROM PRODUIT WHERE ID_PRODUIT = :id");
    query.bindValue(":id", id_produit);

    if (query.exec() && query.next()) {
        return query.value(0).toString();
    }
    return "";
}

bool LigneVente::ajouter()
{
    if (!verifierStockDisponible(id_produit, quantite)) {
        qDebug() << "Stock insuffisant pour le produit" << id_produit;
        return false;
    }

    QSqlQuery query;
    query.prepare("INSERT INTO LIGNE_VENTE (ID_LIGNE, ID_VENTE, ID_PRODUIT, QUANTITE, PRIX_UNITAIRE, REMISE_POURCENT) "
                  "VALUES (SEQ_LIGNE_VENTE.NEXTVAL, :id_vente, :id_produit, :quantite, :prix_unitaire, :remise_pourcent)");
    query.bindValue(":id_vente", id_vente);
    query.bindValue(":id_produit", id_produit);
    query.bindValue(":quantite", quantite);
    query.bindValue(":prix_unitaire", prix_unitaire);
    query.bindValue(":remise_pourcent", remise_pourcent);

    if (!query.exec()) {
        qDebug() << "Erreur ajout ligne vente:" << query.lastError().text();
        return false;
    }

    QSqlQuery updateStock;
    updateStock.prepare("UPDATE PRODUIT SET QUANTITE = QUANTITE - :quantite WHERE ID_PRODUIT = :id_produit");
    updateStock.bindValue(":quantite", quantite);
    updateStock.bindValue(":id_produit", id_produit);
    updateStock.exec();

    return true;
}

bool LigneVente::modifier(int id)
{
    QSqlQuery queryOld;
    queryOld.prepare("SELECT QUANTITE, ID_PRODUIT FROM LIGNE_VENTE WHERE ID_LIGNE = :id");
    queryOld.bindValue(":id", id);

    if (!queryOld.exec() || !queryOld.next()) {
        qDebug() << "Ligne de vente introuvable";
        return false;
    }

    int ancienne_quantite = queryOld.value(0).toInt();
    int ancien_produit = queryOld.value(1).toInt();

    QSqlQuery restoreStock;
    restoreStock.prepare("UPDATE PRODUIT SET QUANTITE = QUANTITE + :quantite WHERE ID_PRODUIT = :id_produit");
    restoreStock.bindValue(":quantite", ancienne_quantite);
    restoreStock.bindValue(":id_produit", ancien_produit);
    restoreStock.exec();

    if (!verifierStockDisponible(id_produit, quantite)) {
        QSqlQuery revert;
        revert.prepare("UPDATE PRODUIT SET QUANTITE = QUANTITE - :quantite WHERE ID_PRODUIT = :id_produit");
        revert.bindValue(":quantite", ancienne_quantite);
        revert.bindValue(":id_produit", ancien_produit);
        revert.exec();
        return false;
    }

    QSqlQuery query;
    query.prepare("UPDATE LIGNE_VENTE SET ID_VENTE = :id_vente, ID_PRODUIT = :id_produit, "
                  "QUANTITE = :quantite, PRIX_UNITAIRE = :prix_unitaire, REMISE_POURCENT = :remise_pourcent "
                  "WHERE ID_LIGNE = :id");
    query.bindValue(":id_vente", id_vente);
    query.bindValue(":id_produit", id_produit);
    query.bindValue(":quantite", quantite);
    query.bindValue(":prix_unitaire", prix_unitaire);
    query.bindValue(":remise_pourcent", remise_pourcent);
    query.bindValue(":id", id);
    query.exec();

    QSqlQuery updateStock;
    updateStock.prepare("UPDATE PRODUIT SET QUANTITE = QUANTITE - :quantite WHERE ID_PRODUIT = :id_produit");
    updateStock.bindValue(":quantite", quantite);
    updateStock.bindValue(":id_produit", id_produit);
    updateStock.exec();

    return true;
}

bool LigneVente::supprimer(int id)
{
    QSqlQuery queryInfo;
    queryInfo.prepare("SELECT QUANTITE, ID_PRODUIT FROM LIGNE_VENTE WHERE ID_LIGNE = :id");
    queryInfo.bindValue(":id", id);

    if (!queryInfo.exec() || !queryInfo.next()) {
        qDebug() << "Ligne de vente introuvable";
        return false;
    }

    int quantite_a_restaurer = queryInfo.value(0).toInt();
    int produit_id = queryInfo.value(1).toInt();

    QSqlQuery query;
    query.prepare("DELETE FROM LIGNE_VENTE WHERE ID_LIGNE = :id");
    query.bindValue(":id", id);
    query.exec();

    QSqlQuery restoreStock;
    restoreStock.prepare("UPDATE PRODUIT SET QUANTITE = QUANTITE + :quantite WHERE ID_PRODUIT = :id_produit");
    restoreStock.bindValue(":quantite", quantite_a_restaurer);
    restoreStock.bindValue(":id_produit", produit_id);
    restoreStock.exec();

    return true;
}

QSqlQueryModel* LigneVente::afficher()
{
    QSqlQueryModel* model = new QSqlQueryModel();
    model->setQuery("SELECT lv.ID_LIGNE, lv.ID_VENTE, lv.ID_PRODUIT, p.NOM_PRODUIT, "
                    "lv.QUANTITE, lv.PRIX_UNITAIRE, lv.REMISE_POURCENT, lv.PRIX_TOTAL_LIGNE "
                    "FROM LIGNE_VENTE lv "
                    "JOIN PRODUIT p ON lv.ID_PRODUIT = p.ID_PRODUIT "
                    "ORDER BY lv.ID_LIGNE DESC");

    model->setHeaderData(0, Qt::Horizontal, "ID Ligne");
    model->setHeaderData(1, Qt::Horizontal, "ID Vente");
    model->setHeaderData(2, Qt::Horizontal, "ID Produit");
    model->setHeaderData(3, Qt::Horizontal, "Nom Produit");
    model->setHeaderData(4, Qt::Horizontal, "Quantité");
    model->setHeaderData(5, Qt::Horizontal, "Prix Unitaire");
    model->setHeaderData(6, Qt::Horizontal, "Remise %");
    model->setHeaderData(7, Qt::Horizontal, "Prix Total");

    // Mettre seulement le texte en noir
    for (int row = 0; row < model->rowCount(); ++row) {
        for (int col = 0; col < model->columnCount(); ++col) {
            model->setData(model->index(row, col), QBrush(Qt::black), Qt::ForegroundRole);
        }
    }

    return model;
}

QSqlQueryModel* LigneVente::afficherParVente(int id_vente)
{
    QSqlQueryModel* model = new QSqlQueryModel();
    QSqlQuery query;
    query.prepare("SELECT lv.ID_LIGNE, lv.ID_VENTE, lv.ID_PRODUIT, p.NOM_PRODUIT, "
                  "lv.QUANTITE, lv.PRIX_UNITAIRE, lv.REMISE_POURCENT, lv.PRIX_TOTAL_LIGNE "
                  "FROM LIGNE_VENTE lv "
                  "JOIN PRODUIT p ON lv.ID_PRODUIT = p.ID_PRODUIT "
                  "WHERE lv.ID_VENTE = :id_vente "
                  "ORDER BY lv.ID_LIGNE");
    query.bindValue(":id_vente", id_vente);
    query.exec();

    model->setQuery(query);

    model->setHeaderData(0, Qt::Horizontal, "ID Ligne");
    model->setHeaderData(1, Qt::Horizontal, "ID Vente");
    model->setHeaderData(2, Qt::Horizontal, "ID Produit");
    model->setHeaderData(3, Qt::Horizontal, "Nom Produit");
    model->setHeaderData(4, Qt::Horizontal, "Quantité");
    model->setHeaderData(5, Qt::Horizontal, "Prix Unitaire");
    model->setHeaderData(6, Qt::Horizontal, "Remise %");
    model->setHeaderData(7, Qt::Horizontal, "Prix Total");

    // Mettre seulement le texte en noir
    for (int row = 0; row < model->rowCount(); ++row) {
        for (int col = 0; col < model->columnCount(); ++col) {
            model->setData(model->index(row, col), QBrush(Qt::black), Qt::ForegroundRole);
        }
    }

    return model;
}

