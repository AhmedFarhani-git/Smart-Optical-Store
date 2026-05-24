#ifndef OPTICALSTORE_H
#define OPTICALSTORE_H

#include <QMainWindow>
#include <QSqlQueryModel>
#include <QTableWidgetItem>
#include <QMap>
#include <QByteArray>
#include "lignevente.h"
// Pour QStyledItemDelegate et QPainter
#include <QStyledItemDelegate>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QModelIndex>
#include <QPen>
#include <QRect>

// Charts
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QChart>

// Dialogs / Layouts / Widgets
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFileDialog>

// Network / JSON (AI API)
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>

#include <fournisseur.h>
#include "gestionstock.h"
#include "gestionemployee.h"
#include "gestionventes.h"
#include "gestionclient.h"
#include "arduino.h"

// Pour QTimer
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui {
class OpticalStore;
}
QT_END_NAMESPACE

// NOUVEAU - Delegate pour afficher les ticks
class TickDelegate : public QStyledItemDelegate
{
public:
    explicit TickDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}


    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};
class NumericItem : public QTableWidgetItem {
public:
    NumericItem(const QString &text) : QTableWidgetItem(text) {}
    bool operator<(const QTableWidgetItem &other) const override {
        return text().toDouble() < other.text().toDouble();
    }
};

class OpticalStore : public QMainWindow
{
    Q_OBJECT

public:
    OpticalStore(QWidget *parent = nullptr);
    ~OpticalStore();
    // Déclaration de la fonction generateQRCode
    void generateQRCode(const QString &data);
    void afficherStatsCategorie();


private slots:
    // === CRUD Fournisseur ===
    void on_ajoutfournisseur_clicked();
    void on_suppfournisseur_clicked();
    void on_updatefournisseur_clicked();
    void on_tablefourni_clicked(const QModelIndex &index);  // Add this declaration
    //recherche
    void on_lineEdit_rechercheFournisseur_textChanged(const QString &text);
    //tri
    void on_comboBox_triFournisseur_currentIndexChanged(const QString &text);
    //export-pdf
    void on_btn_exportFournisseurPDF_clicked();
    //stat
    void on_btnStatistiquesFournisseur_clicked();
    //anomalie
    void detecterAnomalies();
    void on_btnDetecterAnomalies_clicked();
    //qrcode
    void on_pushButtonGenererQRCode_clicked();

    // === GESTION STOCK (Produits) ===
    void ajouterProduit();
    void modifierProduit();
    void supprimerProduit();
    void rechercherProduit(const QString &nom);
    void on_tableWidget_cellClicked(int row, int column);

    // === CRUD EMPLOYES ===
    void on_ajouterEMP_clicked();
    void on_editEMP_clicked();
    void on_suppEMP_clicked();
    void on_tableView_3_clicked(const QModelIndex &index);
    void on_exporterEMP_clicked();
    void on_annulerEMP_clicked();
    void on_rechercherEMP_clicked();
    void on_trierEMP_currentIndexChanged(int index);

    // === GESTION MOT DE PASSE / LOGIN ===
    void on_resetpass_clicked();
    void on_resetforg_clicked();
    void on_forgcon_clicked();
    void on_connectlog_clicked();
    void on_connectlog_2_clicked();

    // === CRUD VENTES ===
    void on_VAjouter_bt_2_clicked();
    void on_VModifier_bt_2_clicked();
    void on_VSupprimer_bt_2_clicked();
    void on_Vreset_bt_2_clicked();
    void on_tableView_clicked(const QModelIndex &index);
    void on_Vprixht_txt_2_textChanged(const QString &text);
    void on_Vremisetxt_2_textChanged(const QString &text);

    // ======== NOUVEAUX SLOTS VENTES/DÉTAILS ========
    void on_VExporterbt_2_clicked();
    void updateVentesStats();
    void on_stat_vente_clicked();
    void on_VtriComboBox_activated(int index);
    void on_Vrecherche_clicked();
    void on_VRecherchetxt_2_textChanged(const QString &text);
    void on_Vdetail_clicked();
    // ======== SLOTS DÉTAILS VENTE ========
    void on_confirmerDetail_clicked();
    void on_dtableView1_clicked(const QModelIndex &index);
    void on_dtableView2_clicked(const QModelIndex &index);
    void on_dtableView3_clicked(const QModelIndex &index);
    void on_quantiteDetail_textChanged(const QString &text);
    void on_initialisationComplete();
    void on_modifierContenir_clicked();
    void on_supprimerContenir_clicked();
    void on_rechercheContenir_clicked();
    void on_rechercheContenirTxt_textChanged(const QString &text);
    void on_exporterContenir_clicked();

    // ======== CRUD CLIENTS ========
    void on_addclient_clicked();
    void on_edit_clicked();
    void on_deleteclient_clicked();
    void afficherClients();
    void viderChampsClient();
    void on_tablelist_cellClicked(int row, int column);

    // ======== NOUVELLES FONCTIONNALITÉS CLIENTS ========
    void on_rechercher_2_clicked();   // recherche clients
    void on_trier_clicked();        // tri clients
    void on_exporter_clicked();     // export PDF clients
    void on_statistiques_clicked(); // stats tranches d'âge

    // ======== GESTION PHOTOS CLIENT ========
    void on_importerPhoto_clicked();
    void on_supprimerPhoto_clicked();

    // ======== RECOMMANDATION AI LUNETTES ========
    void on_recommand_glass_btn_clicked();

    // ======== NAVIGATION ========
    void on_gestionemp_clicked();
    void on_quitcon_clicked();
    void on_QuitterEMP_clicked();
    void on_quiteclients_clicked();
    void on_gclient_clicked();
    void on_quitterlog_clicked();
    void on_accueilfourni_clicked();
    void on_retourhome_clicked();
    void on_retourhome_2_clicked();
    void on_GestionV_bt_clicked();
    void on_quitterlog_2_clicked();
    void on_pushButton_7_clicked();
    void on_btnRetour_clicked();
    void on_btnGestionStock_clicked();
    void on_return_4_clicked();
    void on_return_2_clicked();
    void on_vdetailretour_clicked();

    //eya2
    void exporterProduitsEnPDF();  // <-- déclaration manquante
    void on_comboBoxTri_currentIndexChanged(const QString &critere);
    void on_lineEdit_recherche_textChanged(const QString &texte);
    void rafraichirStockSecurite(); // <- ajouter cette déclaration
    void afficherStockProduits();
    void on_btn_ligne_vente_clicked();
    void on_btn_ajouter_ligne_clicked();
    void on_btn_modifier_ligne_clicked();
    void on_btn_supprimer_ligne_clicked();
    void on_tableLignesVente_cellClicked(int row, int column);
    void remplirTableLignesVente();
    void clearLigneVenteForm();
    int getSelectedLigneID();
    void on_produitSelectionne(int index);
    void on_venteSelectionnee(int index) ;

    void on_btnRetour_2_clicked();
    void remplirTableStockSecurite();
    // Nouveau slot pour gérer la sélection du nom de produit
    void on_comboBox_produit_nom_currentIndexChanged(int index);

    // Nouveau slot pour gérer la sélection de l'ID produit
    void on_comboBox_id_produit_currentIndexChanged(int index);
    void on_comboBox_vente_currentIndexChanged(int index);
    void calculerRemiseAuto(const QString &idClientText);

    // ======== SLOTS CAPTEUR SANS BOUTON ========
    void onObjectDetected();  // Détection par capteur
    void onSensorReady();     // Capteur prêt
    void onArduinoConnected(bool connected);
    void onArduinoError(const QString &error);

    // Slot pour le TextBox d'ID client
    void on_txtIdClientCapteur_textChanged(const QString &text);


private:
    Ui::OpticalStore *ui;


    // Modules métier
    GestionVentes   *gv;
    Fournisseur      f;
    gestionemployee  ge;
    GestionClient   *gc;
    gestionstock stock;
    gestionstock gs;
    LigneVente ligneVente;

    // Arduino
    Arduino *arduino;
    QTimer *detectionTimer;  // Timer pour anti-rebond
    bool canDetect;          // Permettre la détection
    int currentClientIdForDetection;  // ID client actuel dans le TextBox

    // Login / sécurité
    QString currentUserId;
    bool passwordChanged = false;

    // Photo client actuellement chargée
    QByteArray photoActuelle;

    // ====== NOUVELLES VARIABLES POUR VENTES/DÉTAILS ======
    TickDelegate *tickDelegate1;
    TickDelegate *tickDelegate2;
    TickDelegate *tickDelegate3;

    int selectedVenteId;
    int selectedProduitId;
    double selectedProduitPrix;
    bool graphsSwitched;  // Pour le switch des graphiques

    // ====== UTILITAIRES EMPLOYES / STOCK / VENTES ======
    bool validateForm();       // employés
    bool validateInputs();     // produits
    void remplirTable();       // produits
    void clearForm();          // produits
    int  getSelectedProductID();
    void remplirTable_emp();   // employés
    void clearForm_emp();
    int  generateUniqueID();
    void viderChampsVente();
    void calculerPrixTTC();
    void updatePosteCharts();

    // ====== NOUVELLES FONCTIONS VENTES/DÉTAILS ======
    void setupVentesCharts();
    void switchGraphs();
    void updateCurrentChart();
    void showVentesParMois();
    void showRepartitionPrix();
    bool isValidDate(const QString &dateStr);
    void appliquerStyleSelection();
    void afficherDetailsVentes();
    void afficherDetailsProduits();
    void afficherDetailsContenir();
    void calculerTotal();
    void mettreAJourIndicateurs();
    void surlignerLignesCorrespondantes(int idVente, int idProduit);

    // Gestion des droits d'accès selon poste
    void grantResponsableRHAccess();
    void grantResponsableStockAccess();
    void grantResponsableAchatsAccess();
    void grantResponsableRelationClientAccess();
    void grantResponsableCommercialAccess();
    void grantEmployeeAccess();
    void grantAdmintAccess();

    // ====== UTILITAIRES PHOTOS CLIENT ======
    void afficherPhoto(const QByteArray &photoData);
    void viderPhotoInterface();
    void sauvegarderPhotoClient(int idClient);
    void chargerPhotoClient(int idClient);

    // ====== AI / RECO FACE ======
    QString analyzeFaceShape(const QByteArray &imageData);

    void remplirStatsCategorie();
    void synchroniserComboProduits();

    // ====== FONCTIONS CAPTEUR SANS BOUTON ======
    void setupSensorDetection();
    void addFidelityPoints(int clientId, int points = 10);
    void updateClientFidelityDisplay(int clientId, int newPoints);
    bool validateClientForPoints(int clientId);

    void afficherClientSurArduino(int idClient);
    void afficherPointsEtRemise(int idClient);
    void afficherClientEtPoints(int idClient, int idVente);



};

#endif // OPTICALSTORE_H
