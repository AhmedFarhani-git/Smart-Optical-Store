#include "opticalstore.h"
#include "ui_opticalstore.h"

#include "fournisseur.h"
#include "gestionstock.h"
#include "gestionventes.h"
#include "gestionclient.h"
#include "gestionemployee.h"

#include <QMessageBox>
#include <QDebug>
#include <QTableWidgetItem>
#include <QRegularExpression>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QRandomGenerator>
#include <QSqlError>
#include <QFileDialog>
#include <QPdfWriter>
#include <QPainter>
#include <QDateTime>
#include <QIntValidator>
#include <QVBoxLayout>
#include <QLayoutItem>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>
#include <QtCharts/QChart>
#include <QtCharts>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <QTextDocument>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QLineEdit>
#include <QStatusBar>
#include <QDir>
#include <QPixmap>
#include <QBuffer>
#include <QInputDialog>
#include <QPageLayout>
#include <QPageSize>
#include <QSslSocket>

#include <QHttpMultiPart>
#include <QHttpPart>
#include <QSqlRecord>
#include <QTabWidget>
#include <QChartView>
#include <QChart>
#include <QPieSeries>
#include <QPieSlice>
#include <QtCharts/QLegendMarker>
#include <QGraphicsTextItem>




OpticalStore::OpticalStore(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::OpticalStore),
    gv(new GestionVentes()),
    gc(new GestionClient()),
    photoActuelle(QByteArray())
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentWidget(ui->page);
    //afficher-fourni
    ui->tablefourni->setModel(f.afficher());
    //tri
    // tri-fournisseur
    connect(ui->comboBox_triFournisseur, &QComboBox::currentTextChanged,
            this, &OpticalStore::on_comboBox_triFournisseur_currentIndexChanged);
    //tabfourni
    connect(ui->tablefourni, SIGNAL(clicked(const QModelIndex &)),
            this, SLOT(on_tableemploye_clicked(const QModelIndex &)));

    // Debug SSL info (for API)
    qDebug() << "Supports SSL:" << QSslSocket::supportsSsl();
    qDebug() << "SSL Build Version:" << QSslSocket::sslLibraryBuildVersionString();
    qDebug() << "SSL Loaded Version:" << QSslSocket::sslLibraryVersionString();
    qDebug() << "Backends:" << QSslSocket::availableBackends();

    // ==================== INITIALISATION VENTES ====================
    ui->Vdateventetxt->setDisplayFormat("dd/MM/yyyy");
    ui->Vdateventetxt->setDate(QDate::currentDate());
    ui->Vdateventetxt->setCalendarPopup(true);

    ui->Vtvatxt->setText("19");
    ui->Vtvatxt->setReadOnly(true);
    ui->Vprixttctxt_2->setReadOnly(true);

    connect(ui->Vprixht_txt_2, &QLineEdit::textChanged, this, &OpticalStore::calculerPrixTTC);
    connect(ui->Vremisetxt_2, &QLineEdit::textChanged, this, &OpticalStore::calculerPrixTTC);

    // Table ventes
    ui->tableView->setModel(gv->afficher());
    connect(ui->tableView, &QTableView::clicked, this, &OpticalStore::on_tableView_clicked);

    // ==================== INITIALISATION EMPLOYES ====================
    ui->enfantEMP->setValidator(new QIntValidator(0, 9, this));
    ui->tableView_3->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView_3->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableView_3->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableView_3->horizontalHeader()->setStretchLastSection(true);
    ui->tableView_3->resizeColumnsToContents();

    remplirTable_emp();
    ui->IDEMP->setText(QString::number(generateUniqueID()));
    updatePosteCharts();

    // ==================== LOGIN / MDP ====================
    connect(ui->forgcon,   &QPushButton::clicked, this, &OpticalStore::on_forgcon_clicked);
    connect(ui->resetforg, &QPushButton::clicked, this, &OpticalStore::on_resetforg_clicked);

    // Forcer texte noir sur blanc pour tous les QLineEdit
    const QList<QLineEdit*> edits = findChildren<QLineEdit*>();
    for (QLineEdit* edit : edits) {
        if (edit) {
            edit->setStyleSheet("background-color: white; color: black;");
        }
    }

    // Boutons login
    connect(ui->connect,    &QPushButton::clicked, this, &OpticalStore::on_connectlog_clicked);
    connect(ui->resetpass,  &QPushButton::clicked, this, &OpticalStore::on_resetpass_clicked);
    connect(ui->resetpass,  &QPushButton::clicked, this, &OpticalStore::on_resetforg_clicked);

    // ==================== GESTION STOCK (Produits) ====================
    connect(ui->btn_ajouter,   &QPushButton::clicked, this, &OpticalStore::ajouterProduit);
    connect(ui->btn_modifier,  &QPushButton::clicked, this, &OpticalStore::modifierProduit);
    connect(ui->btn_supprimer, &QPushButton::clicked, this, &OpticalStore::supprimerProduit);
    connect(ui->btn_consulter, &QPushButton::clicked, [this]() {
        rechercherProduit(ui->lineEdit_nom->text());
    });
    connect(ui->pushButton_10, &QPushButton::clicked, this, &OpticalStore::afficherStatsCategorie);


    connect(ui->tableWidget_produits, &QTableWidget::cellClicked,
            this, &OpticalStore::on_tableWidget_cellClicked);

    // Remplir table produits
    remplirTable();

    // ==================== CONFIG CLIENTS ====================
    ui->tablelist->setColumnCount(8);
    ui->tablelist->setHorizontalHeaderLabels(
        QStringList() << "ID" << "Nom" << "Prénom" << "Email"
                      << "Téléphone" << "Adresse" << "Date de naissance" << "Fidélité");
    ui->tablelist->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(ui->tablelist, &QTableWidget::cellClicked,
            this, &OpticalStore::on_tablelist_cellClicked);

    // PHOTO client
    ui->labelPhoto->setScaledContents(true);
    ui->labelPhoto->setFixedSize(150, 150);
    ui->labelPhoto->setStyleSheet("QLabel { border: 2px dashed #ccc; background-color: #f9f9f9; }");
    viderPhotoInterface();

    // ==================== AFFICHAGE INITIAL ====================
    afficherClients();

    // ==================== PAGE DE DEMARRAGE ====================
    // 👉 Démarrer directement sur la page Gestion Client (page_4)
    //ui->stackedWidget->setCurrentWidget(ui->page_4);

    tickDelegate1 = new TickDelegate(this);
    tickDelegate2 = new TickDelegate(this);
    tickDelegate3 = new TickDelegate(this);

    graphsSwitched = false;
    selectedVenteId = -1;
    selectedProduitId = -1;
    selectedProduitPrix = 0;

    // Configuration des tableviews pour les détails
    ui->dtableView1->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->dtableView1->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->dtableView2->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->dtableView2->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->dtableView3->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->dtableView3->setSelectionMode(QAbstractItemView::SingleSelection);

    // Appliquer les delegates
    ui->dtableView1->setItemDelegate(tickDelegate1);
    ui->dtableView2->setItemDelegate(tickDelegate2);
    ui->dtableView3->setItemDelegate(tickDelegate3);

    // Appliquer le style
    appliquerStyleSelection();

    // Connexions pour les détails
    connect(ui->dtableView1, &QTableView::clicked, this, &OpticalStore::on_dtableView1_clicked);
    connect(ui->dtableView2, &QTableView::clicked, this, &OpticalStore::on_dtableView2_clicked);
    connect(ui->dtableView3, &QTableView::clicked, this, &OpticalStore::on_dtableView3_clicked);
    connect(ui->quantiteDetail, &QLineEdit::textChanged, this, &OpticalStore::on_quantiteDetail_textChanged);
    connect(ui->VtriComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OpticalStore::on_VtriComboBox_activated);

    // Initialiser les données
    setupVentesCharts();
    updateVentesStats();

    // Initialiser les détails
    afficherDetailsVentes();
    afficherDetailsProduits();
    afficherDetailsContenir();

    // Désactiver les boutons au démarrage
    ui->modifierContenir->setEnabled(false);
    ui->supprimerContenir->setEnabled(false);

    // Lancer une initialisation différée
    QTimer::singleShot(100, this, &OpticalStore::on_initialisationComplete);
    //eya metiers
    connect(ui->btn_exporter_pdf, &QPushButton::clicked, this, &OpticalStore::exporterProduitsEnPDF);
    // ComboBox tri
    connect(ui->comboBox_5, &QComboBox::currentTextChanged,
            this, &OpticalStore::on_comboBoxTri_currentIndexChanged);

    // Recherche dynamique
    connect(ui->lineEdit_recherche, &QLineEdit::textChanged,
            this, &OpticalStore::on_lineEdit_recherche_textChanged);

    rafraichirStockSecurite();
    afficherStockProduits();

    connect(ui->tableLignesVente, &QTableWidget::cellClicked,
            this, &OpticalStore::on_tableLignesVente_cellClicked);

    remplirTable();
    afficherStatsCategorie();
    clearForm();

    //eya
    stock.remplirComboVentesAvecParDefaut(ui->comboBox_vente);
    stock.remplirComboProduitsAvecParDefaut(ui->comboBox_produit_nom);
    ui->comboBox_vente->setCurrentIndex(0);
    ui->comboBox_produit_nom->setCurrentIndex(0);

    connect(ui->comboBox_produit_nom, &QComboBox::currentIndexChanged,
            this, &OpticalStore::on_produitSelectionne);
    connect(ui->comboBox_vente, &QComboBox::currentIndexChanged,
            this, &OpticalStore::on_venteSelectionnee);

    // Ajouter dans opticalstore.h :
    remplirTableLignesVente();
    if (ui->comboBox_produit_nom->count() > 0) {
        on_produitSelectionne(0);
    }
    if (ui->comboBox_vente->count() > 0) {
        on_venteSelectionnee(0);
    }
    connect(ui->btnRetour_2,   &QPushButton::clicked, this, &OpticalStore::on_btnRetour_2_clicked);

        connect(ui->pushButton_11,   &QPushButton::clicked, this, &OpticalStore::remplirTableStockSecurite);

    ui->comboBox_produit_nom->clear();
    ui->comboBox_produit_nom->addItem("Sélectionnez un produit", -1);

    // Charger les produits depuis la base
    QSqlQuery query;
    query.prepare("SELECT ID_PRODUIT, NOM_PRODUIT FROM PRODUIT ORDER BY NOM_PRODUIT ASC");
    if (query.exec()) {
        while (query.next()) {
            int id_produit = query.value(0).toInt();
            QString nom_produit = query.value(1).toString();
            ui->comboBox_produit_nom->addItem(nom_produit, id_produit);
        }
    } else {
        qDebug() << "Erreur lors du chargement des produits:" << query.lastError().text();
    }

        // Définir l'index par défaut
        ui->comboBox_id_produit->setCurrentIndex(0);

    // Connecter les nouveaux signaux
  connect(ui->comboBox_produit_nom, QOverload<int>::of(&QComboBox::currentIndexChanged),
                  this, &OpticalStore::on_comboBox_produit_nom_currentIndexChanged);

        connect(ui->comboBox_id_produit, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &OpticalStore::on_comboBox_id_produit_currentIndexChanged);

    // Synchroniser initialement
    synchroniserComboProduits();
        ui->comboBox_produit_nom->setCurrentIndex(0); // Index 0 = "Sélectionnez un produit"

        // Définir explicitement l'index par défaut pour comboBox_id_produit
        ui->comboBox_id_produit->setCurrentIndex(0); // Index 0 = "Sélectionnez un produit"

        // Définir explicitement l'index par défaut pour comboBox_vente
        ui->comboBox_vente->setCurrentIndex(0); // Index 0 = "Sélectionnez une vente"

        arduino = new Arduino(this);

        connect(arduino, &Arduino::connected, this, &OpticalStore::onArduinoConnected);
        connect(arduino, &Arduino::error, this, &OpticalStore::onArduinoError);
        connect(arduino, &Arduino::dataSent, this, [this](const QString &data) {
            ui->statusbar->showMessage("Arduino: " + data, 2000);
        });

        // Connecter automatiquement
        QTimer::singleShot(1500, this, [this]() {
            if (arduino->connectToArduino()) {
                qDebug() << "✅ Arduino connecté avec succès";
            }
        });

        connect(ui->Vidclienttxt, &QLineEdit::textChanged,
                this, &OpticalStore::calculerRemiseAuto);

        arduino = new Arduino(this);

        // Connecter les signaux Arduino
        connect(arduino, &Arduino::connected, this, &OpticalStore::onArduinoConnected);
        connect(arduino, &Arduino::error, this, &OpticalStore::onArduinoError);
        connect(arduino, &Arduino::dataSent, this, [this](const QString &data) {
            ui->statusbar->showMessage("Arduino: " + data, 2000);
        });
        connect(arduino, &Arduino::objectDetected, this, &OpticalStore::onObjectDetected);
        connect(arduino, &Arduino::sensorReady, this, &OpticalStore::onSensorReady);

        // Initialiser le timer pour anti-rebond (2 secondes)
        detectionTimer = new QTimer(this);
        detectionTimer->setSingleShot(true);
        detectionTimer->setInterval(2000);
        canDetect = true;
        currentClientIdForDetection = -1;

        // Quand le timer expire, réactiver la détection
        connect(detectionTimer, &QTimer::timeout, this, [this]() {
            canDetect = true;
            qDebug() << "✅ Nouvelle détection autorisée";
        });

        // Connecter le TextBox pour l'ID client
        connect(ui->txtIdClientCapteur, &QLineEdit::textChanged,
                this, &OpticalStore::on_txtIdClientCapteur_textChanged);

        // Connecter automatiquement Arduino après un court délai
        QTimer::singleShot(1500, this, [this]() {
            if (arduino->connectToArduino()) {
                qDebug() << "✅ Arduino connecté avec succès";
            }
        });


}

OpticalStore::~OpticalStore()
{
    delete ui;
    delete gv;
    delete gc;
    // Nettoyer les delegates
    delete tickDelegate1;
    delete tickDelegate2;
    delete tickDelegate3;
    delete arduino;
    // Dans OpticalStore::~OpticalStore() :
    delete detectionTimer;

}

// ===================== CRUD FOURNISSEUR =====================

// AJOUTER
void OpticalStore::on_ajoutfournisseur_clicked() {
    //recuperer les infos saisies dans les champs
    int id = ui->lineEdit_idfournisseur->text().toInt();
    QString nom = ui->lineEdit_nomfournisseur->text();
    QString email = ui->lineEdit_emailfournisseur->text();
    QString telephone = ui->lineEdit_telfournisseur->text();
    QString adresse = ui->lineEdit_adressefournisseur->text();
    QString categorie = ui->comboBox_categoriefournisseur->currentText();

    Fournisseur f(id, nom, email, telephone, adresse, categorie); //instancier un objet de la classe fournisseur en utilisant les infos saisis de l'interface
    bool test = f.ajouter(); //inserer l'objet dans le table fourni et recuperer la val de retour de query.exec

    if (test) {
        ui->tablefourni->setModel(f.afficher());
        QMessageBox::information(nullptr, QObject::tr("OK"),
                                 QObject::tr("Ajout effectué.\n"
                                             "Click Cancel to exit."),
                                 QMessageBox::Cancel);
        ui->lineEdit_idfournisseur->clear();
        ui->lineEdit_nomfournisseur->clear();
        ui->lineEdit_emailfournisseur->clear();
        ui->lineEdit_telfournisseur->clear();
        ui->lineEdit_adressefournisseur->clear();
        ui->comboBox_categoriefournisseur->setCurrentIndex(0);
    }
    else {
        QMessageBox::critical(nullptr, QObject::tr("Not OK"),
                              QObject::tr("Ajout non effectué.\n"
                                          "Click Cancel to exit."),
                              QMessageBox::Cancel);
    }

}

// SUPPRIMER
void OpticalStore::on_suppfournisseur_clicked() {
    int id = ui->lineEdit_idfournisseur->text().toInt();
    bool test = f.supprimer(id);

    if (test) {
        ui->tablefourni->setModel(f.afficher());
        QMessageBox::information(nullptr, QObject::tr("OK"),
                                 QObject::tr("Suppression effectuée.\n"
                                             "Click Cancel to exit."),
                                 QMessageBox::Cancel);
    } else {
        QMessageBox::critical(nullptr, QObject::tr("Not OK"),
                              QObject::tr("Suppression non effectuée.\n"
                                          "Click Cancel to exit."),
                              QMessageBox::Cancel);
    }

    ui->lineEdit_idfournisseur->clear();
    ui->lineEdit_nomfournisseur->clear();
    ui->lineEdit_emailfournisseur->clear();
    ui->lineEdit_telfournisseur->clear();
    ui->lineEdit_adressefournisseur->clear();
    ui->comboBox_categoriefournisseur->setCurrentIndex(0);
}

// MODIFIER
void OpticalStore::on_updatefournisseur_clicked() {
    int id = ui->lineEdit_idfournisseur->text().toInt();
    QString nom = ui->lineEdit_nomfournisseur->text();
    QString email = ui->lineEdit_emailfournisseur->text();
    QString telephone = ui->lineEdit_telfournisseur->text();
    QString adresse = ui->lineEdit_adressefournisseur->text();
    QString categorie;

    if (ui->comboBox_categoriefournisseur->currentIndex() <= 0)
        categorie = "";
    else
        categorie = ui->comboBox_categoriefournisseur->currentText().trimmed();

    Fournisseur f(id, nom, email, telephone, adresse, categorie);
    bool test = f.modifier(id);

    if (test) {
        ui->tablefourni->setModel(f.afficher());
        QMessageBox::information(nullptr, QObject::tr("OK"),
                                 QObject::tr("Modification effectuée.\n"
                                             "Click Cancel to exit."),
                                 QMessageBox::Cancel);
        ui->lineEdit_idfournisseur->clear();
        ui->lineEdit_nomfournisseur->clear();
        ui->lineEdit_emailfournisseur->clear();
        ui->lineEdit_telfournisseur->clear();
        ui->lineEdit_adressefournisseur->clear();
        ui->comboBox_categoriefournisseur->setCurrentIndex(0);
    } else {
        QMessageBox::critical(nullptr, QObject::tr("Not OK"),
                              QObject::tr("Modification non effectuée.\n"
                                          "Click Cancel to exit."),
                              QMessageBox::Cancel);
    }


}
//affichage en linedit
void OpticalStore::on_tablefourni_clicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    QAbstractItemModel *model = ui->tablefourni->model();
    int row = index.row();

    // ID fournisseur
    ui->lineEdit_idfournisseur->setText(
        model->index(row, 0).data().toString()
        );

    // Nom fournisseur
    ui->lineEdit_nomfournisseur->setText(
        model->index(row, 1).data().toString()
        );

    // Email fournisseur
    ui->lineEdit_emailfournisseur->setText(
        model->index(row, 2).data().toString()
        );

    // Téléphone fournisseur (si c’est un comboBox)
    ui->lineEdit_telfournisseur->setText(
        model->index(row, 3).data().toString()
        );


    // Adresse fournisseur
    ui->lineEdit_adressefournisseur->setText(
        model->index(row, 4).data().toString()
        );

    // Catégorie fournisseur (comboBox)
    ui->comboBox_categoriefournisseur->setCurrentText(
        model->index(row, 5).data().toString()
        );


}

//RECHERCHER
// -------------------------- RECHERCHE FOURNISSEUR --------------------------
void OpticalStore::on_lineEdit_rechercheFournisseur_textChanged(const QString &text)
{
    Fournisseur f;
    ui->tablefourni->setModel(f.rechercher(text));
}
// -------------------------- TRI FOURNISSEUR --------------------------
void OpticalStore::on_comboBox_triFournisseur_currentIndexChanged(const QString &text)
{
    Fournisseur f;
    if (text.trimmed().isEmpty() || text == "— Sélectionner —") {
        ui->tablefourni->setModel(f.afficher());
        return;
    }

    ui->tablefourni->setModel(f.trier(text));
}
// ---------------- EXPORTER PDF FOURNISSEUR ----------------
void OpticalStore::on_btn_exportFournisseurPDF_clicked()
{
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Exporter en PDF",
        QDir::homePath() + "/fournisseurs_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".pdf",
        "Fichiers PDF (*.pdf)"
        );

    if (filePath.isEmpty())
        return;

    Fournisseur f;
    if (f.exporterPDF(filePath))
        QMessageBox::information(this, "Succès", "Exportation PDF réussie !");
    else
        QMessageBox::critical(this, "Erreur", "Erreur lors de l'exportation du PDF !");
}




// ===================== GESTION STOCK (Produits) =====================

bool OpticalStore::validateInputs()
{
    QString nom = ui->lineEdit_nom->text().trimmed();
    QString marque = ui->lineEdit_marque->text().trimmed();
    QString categorie = ui->comboBox_categorie->currentText().trimmed();
    QString quantiteStr = ui->lineEdit_quantite->text().trimmed();
    QString prixStr = ui->lineEdit_prix->text().trimmed();

    if (nom.isEmpty() || marque.isEmpty() || categorie.isEmpty() ||
        quantiteStr.isEmpty() || prixStr.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Tous les champs doivent être remplis !");
        return false;
    }

    QRegularExpression rxNom("^[A-Za-z][A-Za-z0-9 ]*$");
    if (!rxNom.match(nom).hasMatch()) {
        QMessageBox::warning(this, "Erreur", "Nom du produit invalide !");
        return false;
    }

    QRegularExpression rxMarque("^[A-Za-z ]+$");
    if (!rxMarque.match(marque).hasMatch()) {
        QMessageBox::warning(this, "Erreur", "Marque invalide !");
        return false;
    }

    bool okQuant, okPrix;
    int quantite = quantiteStr.toInt(&okQuant);
    double prix = prixStr.toDouble(&okPrix);

    if (!okQuant || quantite < 0) {
        QMessageBox::warning(this, "Erreur", "Quantité invalide !");
        return false;
    }
    if (!okPrix || prix < 0) {
        QMessageBox::warning(this, "Erreur", "Prix invalide !");
        return false;
    }
    return true;
}

void OpticalStore::remplirTable()
{
    gs.afficherProduits(ui->tableWidget_produits);
    ui->tableWidget_produits->sortByColumn(0, Qt::AscendingOrder);
}

void OpticalStore::clearForm()
{
    ui->lineEdit_id->clear();
    ui->lineEdit_nom->clear();
    ui->lineEdit_marque->clear();
    ui->lineEdit_quantite->clear();
    ui->lineEdit_prix->clear();
    ui->comboBox_categorie->setCurrentIndex(0);
}

int OpticalStore::getSelectedProductID()
{
    int row = ui->tableWidget_produits->currentRow();
    if (row >= 0 && ui->tableWidget_produits->item(row, 0))
        return ui->tableWidget_produits->item(row, 0)->text().toInt();
    return -1;
}

void OpticalStore::ajouterProduit()
{
    if (!validateInputs()) return;

    QString nom = ui->lineEdit_nom->text().trimmed();
    QString marque = ui->lineEdit_marque->text().trimmed();
    QString categorie = ui->comboBox_categorie->currentText().trimmed();
    int quantite = ui->lineEdit_quantite->text().toInt();
    double prix = ui->lineEdit_prix->text().toDouble();
    int id_fournisseur = ui->lineEdit_idf->text().toInt();

    if (gs.ajouterProduit(nom, quantite, marque, categorie, prix, id_fournisseur)) {
        QMessageBox::information(this, "Succès", "Produit ajouté ✅");
        remplirTable();
        clearForm();
    } else {
        QMessageBox::critical(this, "Erreur", "Échec de l'ajout ! Vérifie la base.");
    }
}

void OpticalStore::modifierProduit()
{
    if (!validateInputs()) return;

    int id = getSelectedProductID();
    if (id < 0) return;

    int id_fournisseur = 1;

    if (gs.modifierProduit(id,
                           ui->lineEdit_nom->text(),
                           ui->lineEdit_quantite->text().toInt(),
                           ui->lineEdit_marque->text(),
                           ui->comboBox_categorie->currentText(),
                           ui->lineEdit_prix->text().toDouble(),
                           id_fournisseur)) {
        QMessageBox::information(this, "Succès", "Produit modifié ✅");
        remplirTable();
        clearForm();
    } else {
        QMessageBox::critical(this, "Erreur", "Échec de la modification ! Vérifie la base.");
    }
}

void OpticalStore::supprimerProduit()
{
    int id = getSelectedProductID();
    if (id < 0) return;

    if (gs.supprimerProduit(id)) {
        QMessageBox::information(this, "Succès", "Produit supprimé ✅");
        remplirTable();
        clearForm();
    } else {
        QMessageBox::critical(this, "Erreur", "Échec de la suppression !");
    }
}

void OpticalStore::rechercherProduit(const QString &nom)
{
    gs.rechercherProduit(ui->tableWidget_produits, nom);
}

void OpticalStore::on_tableWidget_cellClicked(int row, int)
{
    if (row < 0) return;
    ui->lineEdit_id->setText(ui->tableWidget_produits->item(row, 0)->text());
    ui->lineEdit_nom->setText(ui->tableWidget_produits->item(row, 1)->text());
    ui->comboBox_categorie->setCurrentText(ui->tableWidget_produits->item(row, 2)->text());
    ui->lineEdit_marque->setText(ui->tableWidget_produits->item(row, 3)->text());
    ui->lineEdit_quantite->setText(ui->tableWidget_produits->item(row, 4)->text());
    ui->lineEdit_prix->setText(ui->tableWidget_produits->item(row, 5)->text());
}

// ===================== CRUD EMPLOYÉS =====================

bool OpticalStore::validateForm()
{
    QString idEmp = ui->IDEMP->text();
    QString nomEmp = ui->nomEMP->text();
    QString prenomEmp = ui->prenomEMP->text();
    QString posteEmp = ui->postComboBox->currentText();
    QString emailEmp = ui->emailEMP->text();
    QString telEmp = ui->telEMP->text();
    QString sexeEmp = ui->sexeEMP_2->text();
    QString situEmp = ui->situEMP->text();
    QString enfantsEmp = ui->enfantEMP->text();
    QString motdepasseEmp = ui->motdepasseEMP->text();
    QString placeEmp = ui->placeEMP->text();
    QString dateEmp = ui->dateEMP->text();
    QString embaucheEmp = ui->embauche_emp->text();

    if (idEmp.isEmpty() || nomEmp.isEmpty() || prenomEmp.isEmpty() ||
        posteEmp.isEmpty() || emailEmp.isEmpty() || telEmp.isEmpty() ||
        sexeEmp.isEmpty() || situEmp.isEmpty() || enfantsEmp.isEmpty() ||
        motdepasseEmp.isEmpty() || placeEmp.isEmpty() ||
        dateEmp.isEmpty() || embaucheEmp.isEmpty()) {
        QMessageBox::warning(this, "Champs vides",
                             "Il y a des champs vides. Veuillez les remplir.");
        return false;
    }

    QRegularExpression regex(R"(^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$)");
    if (!regex.match(emailEmp).hasMatch()) {
        QMessageBox::warning(this, "Email invalide",
                             "Format : example@domaine.com");
        return false;
    }

    QDate naissanceDate = QDate::fromString(dateEmp, "dd/MM/yyyy");
    if (!naissanceDate.isValid()) {
        QMessageBox::warning(this, "Date de Naissance invalide",
                             "Format : dd/MM/yyyy");
        return false;
    }

    QDate embaucheDate = QDate::fromString(embaucheEmp, "dd/MM/yyyy");
    if (!embaucheDate.isValid()) {
        QMessageBox::warning(this, "Date d'Embauche invalide",
                             "Format : dd/MM/yyyy");
        return false;
    }

    return true;
}

void OpticalStore::remplirTable_emp()
{
    QSqlQueryModel *model = ge.afficher();

    if (!model) {
        qDebug() << "Failed to get model!";
        return;
    }

    model->setParent(ui->tableView_3);
    ui->tableView_3->setModel(model);
    ui->tableView_3->resizeColumnsToContents();
}

void OpticalStore::clearForm_emp()
{
    ui->IDEMP->setText(QString::number(generateUniqueID()));
    ui->motdepasseEMP->clear();
    ui->nomEMP->clear();
    ui->prenomEMP->clear();
    ui->postComboBox->setCurrentIndex(0);
    ui->emailEMP->clear();
    ui->telEMP->clear();
    ui->sexeEMP_2->clear();
    ui->situEMP->clear();
    ui->enfantEMP->clear();
    ui->placeEMP->clear();
    ui->dateEMP->clear();
    ui->embauche_emp->clear();
}

int OpticalStore::generateUniqueID()
{
    int id;
    bool exists = true;

    while (exists) {
        id = QRandomGenerator::global()->bounded(100000, 999999);
        QSqlQuery checkQuery;
        checkQuery.prepare("SELECT COUNT(*) FROM EMPLOYES WHERE ID_EMPLOYE = :id");
        checkQuery.bindValue(":id", id);

        if (checkQuery.exec() && checkQuery.next()) {
            exists = (checkQuery.value(0).toInt() > 0);
        } else {
            qDebug() << "Erreur vérification ID:" << checkQuery.lastError().text();
            exists = false;
        }
    }

    return id;
}

void OpticalStore::on_ajouterEMP_clicked()
{
    if (validateForm()) {
        QString poste = ui->postComboBox->currentText().trimmed();
        QString embaucheDate = ui->embauche_emp->text().trimmed();

        bool ok = ge.ajouter(
            ui->IDEMP->text().toInt(),
            ui->nomEMP->text(),
            ui->prenomEMP->text(),
            poste,
            ui->emailEMP->text(),
            ui->telEMP->text(),
            ui->sexeEMP_2->text(),
            ui->situEMP->text(),
            ui->enfantEMP->text().toInt(),
            ui->motdepasseEMP->text(),
            ui->placeEMP->text(),
            ui->dateEMP->text(),
            embaucheDate
            );

        if (ok) {
            QMessageBox::information(this, "Succès", "Employé ajouté !");
            remplirTable_emp();
            clearForm_emp();
            ui->IDEMP->setFocus();
            updatePosteCharts();
        } else {
            QMessageBox::critical(this, "Erreur", "Échec d’ajout !");
        }
    }
}

void OpticalStore::on_editEMP_clicked()
{
    if (validateForm()) {
        QString email = ui->emailEMP->text().trimmed();
        QString embaucheDate = ui->embauche_emp->text().trimmed();

        bool ok = ge.modifier(
            ui->IDEMP->text().toInt(),
            ui->nomEMP->text(),
            ui->prenomEMP->text(),
            ui->postComboBox->currentText(),
            email,
            ui->telEMP->text(),
            ui->sexeEMP_2->text(),
            ui->situEMP->text(),
            ui->enfantEMP->text().toInt(),
            ui->motdepasseEMP->text(),
            ui->placeEMP->text(),
            ui->dateEMP->text(),
            embaucheDate
            );

        if (ok) {
            QMessageBox::information(this, "Succès", "Employé modifié !");
            remplirTable_emp();
            clearForm_emp();
        } else {
            QMessageBox::critical(this, "Erreur", "Échec de modification !");
        }
    }
}

void OpticalStore::on_suppEMP_clicked()
{
    int id = ui->IDEMP->text().toInt();
    if (ge.supprimer(id)) {
        QMessageBox::information(this, "Succès", "Employé supprimé !");
        remplirTable_emp();
        clearForm_emp();
    } else {
        QMessageBox::critical(this, "Erreur", "Échec de suppression !");
    }
}

void OpticalStore::on_annulerEMP_clicked()
{
    clearForm_emp();
    ui->IDEMP->setFocus();
}

void OpticalStore::on_tableView_3_clicked(const QModelIndex &index)
{
    if (!index.isValid()) return;

    const QAbstractItemModel *m = ui->tableView_3->model();
    int r = index.row();

    ui->IDEMP->setText(m->data(m->index(r, 0)).toString());
    ui->nomEMP->setText(m->data(m->index(r, 1)).toString());
    ui->prenomEMP->setText(m->data(m->index(r, 2)).toString());
    ui->postComboBox->setCurrentText(m->data(m->index(r, 3)).toString());
    ui->emailEMP->setText(m->data(m->index(r, 4)).toString());
    ui->telEMP->setText(m->data(m->index(r, 5)).toString());
    ui->sexeEMP_2->setText(m->data(m->index(r, 6)).toString());
    ui->situEMP->setText(m->data(m->index(r, 7)).toString());
    ui->enfantEMP->setText(m->data(m->index(r, 8)).toString());
    ui->motdepasseEMP->setText(m->data(m->index(r, 9)).toString());
    ui->placeEMP->setText(m->data(m->index(r, 10)).toString());
    ui->dateEMP->setText(m->data(m->index(r, 11)).toString());
    ui->embauche_emp->setText(m->data(m->index(r, 12)).toString());
}

void OpticalStore::on_exporterEMP_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Exporter en PDF",
        "Employes_" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmm") + ".pdf",
        "PDF (*.pdf)"
        );

    if (fileName.isEmpty())
        return;

    QString html;
    html += "<html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<style>";
    html += "body { font-family: Arial; }";
    html += "h1 { text-align: center; color: #333; }";
    html += "table { width: 100%; border-collapse: collapse; margin-top: 20px; }";
    html += "th { background: #f7d98a; padding: 8px; border: 1px solid #999; }";
    html += "td { padding: 6px; border: 1px solid #999; font-size: 10pt; }";
    html += "tr:nth-child(even) { background: #f5f5f5; }";
    html += "tr:nth-child(odd)  { background: #ffffff; }";
    html += "</style></head><body>";
    html += "<h1>Liste des Employés</h1>";
    html += "<p><b>Date d'export :</b> " +
            QDateTime::currentDateTime().toString("dd/MM/yyyy hh:mm") + "</p>";
    html += "<table>";
    html += "<tr><th>ID</th><th>Nom</th><th>Prénom</th><th>Poste</th><th>Email</th>"
            "<th>Téléphone</th><th>Sexe</th><th>Situation</th><th>Enfants</th>"
            "<th>MDP</th><th>Naissance</th><th>Date</th></tr>";

    QSqlQuery q(
        "SELECT ID_EMPLOYE, NOM_EMPLOYE, PRENOM_EMPLOYE, POSTE_EMPLOYE, EMAIL_EMPLOYE, "
        "TELEPHONE_EMPLOYE, SEXE_EMPLOYE, SITUATION_FAMILIALE_EMPLOYE, "
        "NBR_ENFANTS_EMPLOYE, MDP_EMPLOYE, PLACE_EMPLOYE, DATE_EMPLOYE "
        "FROM EMPLOYES ORDER BY ID_EMPLOYE ASC"
        );

    while (q.next()) {
        html += "<tr>";
        for (int i = 0; i < 12; i++)
            html += "<td>" + q.value(i).toString() + "</td>";
        html += "</tr>";
    }

    html += "</table>";
    html += "<p style='margin-top:40px; font-size:10pt;'>Smart Optical Store.</p>";
    html += "</body></html>";

    QTextDocument doc;
    doc.setHtml(html);

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFileName(fileName);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setPageOrientation(QPageLayout::Landscape);

    doc.print(&printer);

    QMessageBox::information(this, "PDF", "Exportation PDF réussie !");
}
//stat
void OpticalStore::on_btnStatistiquesFournisseur_clicked()
{
    Fournisseur f;
    QSqlQueryModel *model = f.statistiquesParCategorie();

    if (model->rowCount() == 0) {
        QMessageBox::information(this, "Statistiques", "Aucun fournisseur trouvé pour générer les statistiques.");
        return;
    }

    QPieSeries *series = new QPieSeries();

    int totalFournisseurs = 0;
    for (int i = 0; i < model->rowCount(); ++i) {
        totalFournisseurs += model->record(i).value("total").toInt();
    }

    for (int i = 0; i < model->rowCount(); ++i) {
        QString categorie = model->record(i).value("categorie").toString();
        int total = model->record(i).value("total").toInt();
        double pourcentage = (totalFournisseurs > 0) ? (total * 100.0 / totalFournisseurs) : 0;

        // Correction d'affichage seulement pour cette catégorie
        if (categorie.contains("ÉQUIPEMENTS")) {
            categorie = "ÉQUIPEMENTS & PRODUITS D’ENTRETIEN";
        }

        QString label = QString("%1 — %2 fournisseurs (%3%)")
                            .arg(categorie)
                            .arg(total)
                            .arg(QString::number(pourcentage, 'f', 1));


        series->append(label, total);
    }

    // 🎨 Couleurs personnalisées
    QStringList couleurs = {"#3C096C", "#5A189A", "#7B2CBF", "#9D4EDD", "#C77DFF"};

    int index = 0;
    for (auto slice : series->slices()) {
        slice->setBrush(QBrush(QColor(couleurs.at(index % couleurs.size()))));
        slice->setLabelVisible(true);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        index++;
    }

    // ✨ Effet de survol dynamique
    for (auto slice : series->slices()) {
        QObject::connect(slice, &QPieSlice::hovered, [slice](bool hovered) {
            slice->setExploded(hovered);
            slice->setBorderWidth(hovered ? 3 : 1);
            slice->setBorderColor(hovered ? QColor("#341671") : Qt::gray);
        });
    }

    // 🟦 Création et style du graphique
    QChart *chart = new QChart();
    chart->addSeries(series);


    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->setTitleBrush(QBrush(Qt::darkBlue));
    chart->setTitleFont(QFont("Poppins", 11, QFont::Bold));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setTheme(QChart::ChartThemeLight);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumSize(500, 300);

    // 🔄 Remplacer l'ancien graphique si existant
    QLayoutItem *item;
    while ((item = ui->layoutStatistiques->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    ui->layoutStatistiques->addWidget(chartView);
}


//email
void OpticalStore::on_btnDetecterAnomalies_clicked()
{
    detecterAnomalies();
}


void OpticalStore::detecterAnomalies()
{
    QSqlQuery q;

    // Vérification
    QSqlQuery test;
    if (!test.exec("SELECT COUNT(*) FROM OPTICALSTORE.PRODUIT")) {
        QMessageBox::warning(this, "Erreur",
                             "Impossible de lire OPTICALSTORE.PRODUIT.\nErreur : "
                                 + test.lastError().text());
        return;
    }
    test.next();
    qDebug() << "Produits trouvés dans OPTICALSTORE =" << test.value(0).toInt();


    // --- Moyenne globale ---
    if (!q.exec("SELECT AVG(PRIX_UNITAIRE) FROM OPTICALSTORE.PRODUIT")) {
        QMessageBox::warning(this, "Erreur", "Impossible de calculer la moyenne globale.");
        return;
    }

    q.next();
    double moyenneGlobale = q.value(0).toDouble();
    double seuilHaut = moyenneGlobale * 1.20;
    double seuilBas  = moyenneGlobale * 0.80;


    // --- Moyenne par fournisseur ---
    if (!q.exec(
            "SELECT ID_FOURNISSEUR, AVG(PRIX_UNITAIRE) "
            "FROM OPTICALSTORE.PRODUIT "
            "GROUP BY ID_FOURNISSEUR"))
    {
        QMessageBox::warning(this, "Erreur", "Erreur moyenne par fournisseur.");
        return;
    }

    bool found = false;
    QString rapport = "<h3>Rapport d’anomalies détectées</h3>";

    while (q.next())
    {
        int idf = q.value(0).toInt();
        double moy = q.value(1).toDouble();

        if (moy > seuilHaut || moy < seuilBas)
        {
            found = true;

            // Info fournisseur
            QSqlQuery q2;
            q2.prepare(
                "SELECT NOM_FOURNISSEUR, EMAIL_FOURNISSEUR "
                "FROM OPTICALSTORE.FOURNISSEURS "
                "WHERE ID_FOURNISSEUR = :id");
            q2.bindValue(":id", idf);
            q2.exec();
            q2.next();

            QString nom   = q2.value(0).toString();
            QString email = q2.value(1).toString();

            QString message;

            if (moy > seuilHaut) {
                message =
                    "Bonjour " + nom + ",<br><br>"
                                       "Votre prix moyen est <b>trop élevé</b> (" + QString::number(moy) + " DT).";
            } else {
                message =
                    "Bonjour " + nom + ",<br><br>"
                                       "Votre prix moyen est <b>trop bas</b> (" + QString::number(moy) + " DT).";
            }

            Fournisseur f;
            f.envoyerEmailAlerte(email, nom, message);

            rapport += "<p>• Fournisseur <b>" + nom + "</b> ➜ Prix moyen : "
                                                      "<b>" + QString::number(moy) + " DT</b></p>";
        }
    }

    if (!found)
        QMessageBox::information(this, "Anomalies", "Aucune anomalie détectée.");
    else
        QMessageBox::information(this, "Anomalies détectées", rapport);
}



//qrcode
#include "qrcodegen.hpp"
#include <QImage>
#include <QPainter>
#include <QInputDialog>  // Ajouter cette ligne pour inclure QInputDialog


#include <QSqlQuery>

void OpticalStore::generateQRCode(const QString &data)
{
    // Convertir le texte en QString en UTF-8
    QByteArray dataUtf8 = data.toUtf8();

    // Créer un QR code à partir du texte
    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(dataUtf8.constData(), qrcodegen::QrCode::Ecc::LOW);

    // Taille de l'image à générer
    int size = qr.getSize();
    int scale = 10; // Facteur de mise à l'échelle pour l'image générée

    // Créer une image QImage avec la taille appropriée
    QImage image(size * scale, size * scale, QImage::Format_RGB888);
    image.fill(Qt::white);  // Initialiser avec un fond blanc

    // Remplir l'image avec les modules du QR code
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            if (qr.getModule(x, y)) {
                // Noir pour les modules activés
                for (int dx = 0; dx < scale; ++dx) {
                    for (int dy = 0; dy < scale; ++dy) {
                        image.setPixel((x * scale) + dx, (y * scale) + dy, qRgb(0, 0, 0));
                    }
                }
            } else {
                // Blanc pour les modules non activés (cela est déjà fait avec fill(Qt::white))
            }
        }
    }

    // Afficher l'image dans le QLabel (ajoutez cette QLabel dans l'UI pour afficher le QR code)
    ui->labelQRCode->setPixmap(QPixmap::fromImage(image).scaled(200, 200, Qt::KeepAspectRatio));
}



void OpticalStore::on_pushButtonGenererQRCode_clicked()
{
    // 1) Récupérer l’ID directement depuis ton lineEdit
    QString idFournisseur = ui->lineEdit_idfournisseur->text().trimmed();

    if (idFournisseur.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner un fournisseur dans le tableau.");
        return;
    }

    // 2) Vérifier que le fournisseur existe dans PRODUIT
    QSqlQuery query;
    query.prepare("SELECT NOM_PRODUIT, PRIX_UNITAIRE FROM PRODUIT WHERE ID_FOURNISSEUR = :id");
    query.bindValue(":id", idFournisseur);

    if (!query.exec()) {
        QMessageBox::warning(this, "Erreur SQL", query.lastError().text());
        return;
    }

    QString produitInfo;

    while (query.next()) {
        QString nomProduit = query.value(0).toString();
        QString prixProduit = query.value(1).toString();
        produitInfo += "Produit : " + nomProduit + "\n" +
                       "Prix : " + prixProduit + "\n\n";
    }

    if (produitInfo.isEmpty()) {
        QMessageBox::warning(this, "Aucun produit", "Aucun produit trouvé pour ce fournisseur.");
        return;
    }

    // 3) Génération du QR code directement dans ton QLabel
    generateQRCode(produitInfo);
}


// ===================== MDP OUBLIE / LOGIN =====================

void OpticalStore::on_resetforg_clicked()
{
    QString date = ui->dateforg->text().trimmed();
    QString place = ui->placeforg->text().trimmed();
    QString enteredId = ui->IDpass->text().trimmed();

    if (date.isEmpty() || place.isEmpty() || enteredId.isEmpty()) {
        QMessageBox::warning(this, "Champs manquants",
                             "Veuillez remplir l'ID, la date et le lieu de naissance.");
        return;
    }

    QSqlQuery q;
    q.prepare("SELECT ID_EMPLOYE FROM EMPLOYES "
              "WHERE ID_EMPLOYE = :id AND DATE_EMPLOYE = :date AND PLACE_EMPLOYE = :place");
    q.bindValue(":id", enteredId);
    q.bindValue(":date", date);
    q.bindValue(":place", place);

    if (q.exec() && q.next()) {
        currentUserId = q.value(0).toString();
        QMessageBox::information(this, "Succès", "Identité vérifiée !");
        ui->stackedWidget->setCurrentWidget(ui->page_8);
    } else {
        QMessageBox::critical(this, "Erreur", "Aucun employé correspondant.");
    }
}

void OpticalStore::on_connectlog_clicked()
{
    QString enteredId = ui->IDcon->text().trimmed();
    QString enteredPassword = ui->passcon->text().trimmed();

    if (enteredId.isEmpty() || enteredPassword.isEmpty()) {
        QMessageBox::warning(this, "Champs vides",
                             "Veuillez entrer un identifiant et un mot de passe.");
        return;
    }

    QSqlQuery query;
    query.prepare("SELECT NOM_EMPLOYE, PRENOM_EMPLOYE, POSTE_EMPLOYE FROM EMPLOYES "
                  "WHERE ID_EMPLOYE = :id AND MDP_EMPLOYE = :password");
    query.bindValue(":id", enteredId);
    query.bindValue(":password", enteredPassword);

    if (query.exec() && query.next()) {

        QString lastName = query.value("NOM_EMPLOYE").toString();
        QString firstName = query.value("PRENOM_EMPLOYE").toString();
        QString poste = query.value("POSTE_EMPLOYE").toString();

        QString fullName = firstName + " " + lastName;
        ui->user->setText("Bienvenue " + fullName);

        if (poste == "Responsable RH")                 grantResponsableRHAccess();
        else if (poste == "Responsable Stock")        grantResponsableStockAccess();
        else if (poste == "Responsable Achats")       grantResponsableAchatsAccess();
        else if (poste == "Responsable de la Relation Client") grantResponsableRelationClientAccess();
        else if (poste == "Responsable Vente")   grantResponsableCommercialAccess();
        else if (poste == "Admin") grantAdmintAccess();
        else grantEmployeeAccess();

        QMessageBox::information(this, "Connexion réussie",
                                 "Bienvenue " + fullName + " !");

        ui->stackedWidget->setCurrentWidget(ui->page_2);

        QRect endRect = ui->frame_2->geometry();
        QRect startRect = endRect;
        startRect.moveTop(endRect.top() + 80);

        QPropertyAnimation *anim = new QPropertyAnimation(ui->frame_2, "geometry");
        anim->setDuration(500);
        anim->setStartValue(startRect);
        anim->setEndValue(endRect);
        anim->setEasingCurve(QEasingCurve::OutCubic);
        anim->start();
    }
    else {
        QMessageBox::critical(this, "Échec de la connexion",
                              "Veuillez vérifier votre identifiant et votre mot de passe.");
    }
}

void OpticalStore::on_resetpass_clicked()
{
    QString newPassword = ui->newpass->text().trimmed();
    QString confirm = ui->confirmpass->text().trimmed();

    if (newPassword.isEmpty() || confirm.isEmpty()) {
        QMessageBox::warning(this, "Champs vides",
                             "Veuillez remplir tous les champs.");
        return;
    }

    if (newPassword != confirm) {
        QMessageBox::warning(this, "Erreur",
                             "Les mots de passe ne correspondent pas.");
        return;
    }

    if (currentUserId.isEmpty()) {
        QMessageBox::critical(this, "Erreur", "Utilisateur introuvable.");
        return;
    }

    QSqlQuery q;
    q.prepare("UPDATE EMPLOYES SET MDP_EMPLOYE = :mdp WHERE ID_EMPLOYE = :id");
    q.bindValue(":mdp", newPassword);
    q.bindValue(":id", currentUserId);

    if (q.exec()) {
        QMessageBox::information(this, "Succès", "Mot de passe changé !");
        ui->stackedWidget->setCurrentWidget(ui->page);
    } else {
        QMessageBox::critical(this, "Erreur",
                              "Échec lors de la modification.");
    }
}

void OpticalStore::on_rechercherEMP_clicked()
{
    QString text = ui->rechercher->text().trimmed();

    QSqlQueryModel *model;

    if (text.isEmpty()) {
        model = ge.afficher();
    } else {
        model = ge.rechercher(text);
    }

    ui->tableView_3->setModel(model);
    ui->tableView_3->resizeColumnsToContents();
}

void OpticalStore::on_trierEMP_currentIndexChanged(int)
{
    QString choix = ui->trierEMP->currentText().trimmed().toLower();
    QSqlQueryModel *model = nullptr;

    if (choix == "a-z") {
        model = ge.trierAZ();
    } else if (choix == "date d'embauche") {
        model = ge.trierAnciennete();
    }

    if (model) {
        ui->tableView_3->setModel(model);
        ui->tableView_3->resizeColumnsToContents();
    }
}

void OpticalStore::updatePosteCharts()
{
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(ui->groupBoxposte->layout());
    if (!layout) {
        layout = new QVBoxLayout(ui->groupBoxposte);
        ui->groupBoxposte->setLayout(layout);
    }

    QLayoutItem *item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    QMap<QString, int> posteCounts;
    QStringList postes;

    QAbstractItemModel *model = ui->tableView_3->model();
    int rows = model ? model->rowCount() : 0;

    for (int r = 0; r < rows; r++) {
        QString poste = model->data(model->index(r, 3)).toString();
        if (!poste.isEmpty()) {
            posteCounts[poste]++;
            if (!postes.contains(poste)) postes.append(poste);
        }
    }

    QPieSeries *pie = new QPieSeries();
    for (const QString &poste : postes) {
        pie->append(poste, posteCounts.value(poste));
    }

    QList<QColor> colors = {
        QColor("#FF7043"),
        QColor("#FFB74D"),
        QColor("#FFEB3B"),
        QColor("#4CAF50"),
        QColor("#1976D2"),
        QColor("#0288D1"),
        QColor("#8E24AA"),
        QColor("#F44336")
    };

    int index = 0;
    for (auto slice : pie->slices()) {
        slice->setBrush(colors[index % colors.size()]);
        slice->setLabel(QString("%1 (%2)").arg(slice->label()).arg(slice->value()));
        slice->setLabelVisible(true);
        slice->setLabelFont(QFont("Arial", 9));
        index++;
    }

    QChart *chart = new QChart();
    chart->addSeries(pie);
    chart->setTitle("Répartition des employés par poste");
    chart->setTitleFont(QFont("Arial", 11, QFont::Bold));
    chart->legend()->setAlignment(Qt::AlignLeft);
    chart->legend()->setFont(QFont("Arial", 9));
    chart->legend()->setMarkerShape(QLegend::MarkerShapeRectangle);
    chart->layout()->setContentsMargins(5, 5, 5, 5);
    chart->setMargins(QMargins(0, 0, 0, 0));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setAnimationDuration(1000);
    chart->setAnimationEasingCurve(QEasingCurve::OutBounce);

    QChartView *view = new QChartView(chart);
    view->setRenderHint(QPainter::Antialiasing);
    view->setFixedSize(950, 250);

    layout->setAlignment(Qt::AlignCenter);
    layout->addWidget(view);
}

void OpticalStore::grantResponsableRHAccess()
{
    ui->gestionemp->setEnabled(true);
    ui->gclient->setEnabled(false);
    ui->accueilfourni->setEnabled(false);
    ui->btnGestionStock->setEnabled(false);
    ui->GestionV_bt->setEnabled(false);
}

void OpticalStore::grantResponsableStockAccess()
{
    ui->gestionemp->setEnabled(false);
    ui->gclient->setEnabled(false);
    ui->accueilfourni->setEnabled(false);
    ui->btnGestionStock->setEnabled(true);
    ui->GestionV_bt->setEnabled(false);
}

void OpticalStore::grantResponsableAchatsAccess()
{
    ui->gestionemp->setEnabled(false);
    ui->gclient->setEnabled(false);
    ui->accueilfourni->setEnabled(true);
    ui->btnGestionStock->setEnabled(false);
    ui->GestionV_bt->setEnabled(false);
}

void OpticalStore::grantResponsableRelationClientAccess()
{
    ui->gestionemp->setEnabled(false);
    ui->gclient->setEnabled(true);
    ui->accueilfourni->setEnabled(false);
    ui->btnGestionStock->setEnabled(false);
    ui->GestionV_bt->setEnabled(false);
}

void OpticalStore::grantResponsableCommercialAccess()
{
    ui->gestionemp->setEnabled(false);
    ui->gclient->setEnabled(false);
    ui->accueilfourni->setEnabled(false);
    ui->btnGestionStock->setEnabled(false);
    ui->GestionV_bt->setEnabled(true);
}

void OpticalStore::grantAdmintAccess()
{
    ui->gestionemp->setEnabled(true);
    ui->gclient->setEnabled(true);
    ui->accueilfourni->setEnabled(true);
    ui->btnGestionStock->setEnabled(true);
    ui->GestionV_bt->setEnabled(true);
}
void OpticalStore::grantEmployeeAccess()
{
    ui->gestionemp->setEnabled(false);
    ui->gclient->setEnabled(false);
    ui->accueilfourni->setEnabled(false);
    ui->btnGestionStock->setEnabled(false);
    ui->GestionV_bt->setEnabled(false);
}
void OpticalStore::on_forgcon_clicked()
{
    passwordChanged = false;
    ui->dateforg->clear();
    ui->placeforg->clear();
    ui->stackedWidget->setCurrentWidget(ui->page_6);
}

// ===================== CRUD VENTES =====================

void OpticalStore::on_VAjouter_bt_2_clicked()
{
    if (ui->Vidventetxt_2->text().isEmpty() || ui->Vdateventetxt->text().isEmpty()) {
        QMessageBox::warning(this, "Champs manquants",
                             "ID Vente et Date sont obligatoires!");
        return;
    }

    int id_vente = ui->Vidventetxt_2->text().toInt();
    QString date_vente = ui->Vdateventetxt->date().toString("dd/MM/yyyy");
    double prix_ht = ui->Vprixht_txt_2->text().toDouble();
    double tva = ui->Vtvatxt->text().toDouble();

    // REMISE CALCULÉE AUTOMATIQUEMENT (lecture seule)
    double remise = ui->Vremisetxt_2->text().toDouble();

    double prix_ttc = ui->Vprixttctxt_2->text().toDouble();
    int id_client = ui->Vidclienttxt->text().toInt();
    int id_employe = ui->Vidvendeurtxt_2->text().toInt();

    if (id_vente <= 0) {
        QMessageBox::warning(this, "ID invalide",
                             "L'ID Vente doit être un nombre positif!");
        return;
    }
    if (prix_ht < 0 || tva < 0 || remise < 0 || prix_ttc < 0) {
        QMessageBox::warning(this, "Valeurs invalides",
                             "Les valeurs monétaires ne peuvent pas être négatives!");
        return;
    }

    gv->setIdVente(id_vente);
    gv->setDateVente(date_vente);
    gv->setPrixHT(prix_ht);
    gv->setTVA(tva);
    gv->setRemise(remise);  // Remise calculée automatiquement
    gv->setPrixTTC(prix_ttc);
    gv->setIdClient(id_client);
    gv->setIdEmploye(id_employe);

    bool test = gv->ajouter();

    if (test) {
        QMessageBox::information(this, "Succès", "Vente ajoutée avec succès !");

        // Afficher sur Arduino avec remise calculée
        afficherClientEtPoints(id_client, id_vente);

        ui->tableView->setModel(gv->afficher());
        viderChampsVente();
    } else {
        QMessageBox::critical(this, "Erreur",
                              "L'ajout de la vente a échoué. Vérifiez les données.");
    }
}

void OpticalStore::on_VSupprimer_bt_2_clicked()
{
    if (ui->Vidventetxt_2->text().isEmpty()) {
        QMessageBox::warning(this, "Champ manquant",
                             "Veuillez saisir un ID Vente à supprimer!");
        return;
    }

    int id_vente = ui->Vidventetxt_2->text().toInt();

    if (id_vente <= 0) {
        QMessageBox::warning(this, "ID invalide",
                             "L'ID Vente doit être un nombre positif!");
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirmation",
                                  "Êtes-vous sûr de vouloir supprimer la vente ID: " +
                                      QString::number(id_vente) + "?",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

    bool test = gv->supprimer(id_vente);

    if (test) {
        QMessageBox::information(this, "Succès",
                                 "Vente supprimée avec succès !");
        ui->tableView->setModel(gv->afficher());
        viderChampsVente();
    } else {
        QMessageBox::critical(this, "Erreur",
                              "La suppression a échoué. Vérifiez que l'ID existe.");
    }
}

void OpticalStore::on_VModifier_bt_2_clicked()
{
    if (ui->Vidventetxt_2->text().isEmpty()) {
        QMessageBox::warning(this, "Champ manquant",
                             "ID Vente est obligatoire pour la modification!");
        return;
    }

    int id_vente = ui->Vidventetxt_2->text().toInt();
    QString date_vente = ui->Vdateventetxt->date().toString("dd/MM/yyyy");
    double prix_ht = ui->Vprixht_txt_2->text().toDouble();
    double tva = ui->Vtvatxt->text().toDouble();

    // REMISE CALCULÉE AUTOMATIQUEMENT (lecture seule)
    double remise = ui->Vremisetxt_2->text().toDouble();

    double prix_ttc = ui->Vprixttctxt_2->text().toDouble();
    int id_client = ui->Vidclienttxt->text().toInt();
    int id_employe = ui->Vidvendeurtxt_2->text().toInt();

    bool test = gv->modifier(id_vente, date_vente, prix_ht, tva, remise, prix_ttc,
                             id_client, id_employe);

    if (test) {
        QMessageBox::information(this, "Succès",
                                 "Vente modifiée avec succès !");

        // Rafraîchir l'affichage Arduino
        afficherClientEtPoints(id_client, id_vente);

        ui->tableView->setModel(gv->afficher());
    } else {
        QMessageBox::critical(this, "Erreur",
                              "La modification a échoué. Vérifiez les données.");
    }
}
void OpticalStore::on_tableView_clicked(const QModelIndex &index)
{
    int row = index.row();

    // Récupérer les données
    QString dateFromDb = index.sibling(row, 1).data().toString();
    QDate date;

    if (dateFromDb.contains('T')) {
        dateFromDb = dateFromDb.split('T').first();
        date = QDate::fromString(dateFromDb, "yyyy-MM-dd");
    } else if (dateFromDb.contains('/')) {
        date = QDate::fromString(dateFromDb, "dd/MM/yyyy");
    } else {
        date = QDate::fromString(dateFromDb, "yyyy-MM-dd");
    }

    ui->Vidventetxt_2->setText(index.sibling(row, 0).data().toString());
    if (date.isValid()) {
        ui->Vdateventetxt->setDate(date);
    } else {
        ui->Vdateventetxt->setDate(QDate::currentDate());
    }

    ui->Vprixht_txt_2->setText(index.sibling(row, 2).data().toString());
    ui->Vtvatxt->setText(index.sibling(row, 3).data().toString());
    ui->Vprixttctxt_2->setText(index.sibling(row, 5).data().toString());

    // ID client - déclenchera calcul automatique
    ui->Vidclienttxt->setText(index.sibling(row, 6).data().toString());

    ui->Vidvendeurtxt_2->setText(index.sibling(row, 7).data().toString());

}

void OpticalStore::viderChampsVente()
{
    ui->Vidventetxt_2->clear();
    ui->Vdateventetxt->setDate(QDate::currentDate());
    ui->Vprixht_txt_2->clear();
    ui->Vtvatxt->setText("19");
    ui->Vremisetxt_2->setText("0");
    ui->Vprixttctxt_2->clear();
    ui->Vidclienttxt->clear();
    ui->Vidvendeurtxt_2->clear();
}

void OpticalStore::on_Vreset_bt_2_clicked()
{
    viderChampsVente();
}

void OpticalStore::calculerPrixTTC()
{
    double prix_ht = ui->Vprixht_txt_2->text().toDouble();
    double tva = ui->Vtvatxt->text().toDouble();

    // LIRE LA REMISE DU CHAMP (qui est maintenant calculée automatiquement)
    double remise = ui->Vremisetxt_2->text().toDouble();

    double prix_ttc = 0.0;

    if (remise > 0) {
        double prix_apres_remise = prix_ht - (prix_ht * remise / 100);
        prix_ttc = prix_apres_remise * (1 + tva / 100);
    } else {
        prix_ttc = prix_ht * (1 + tva / 100);
    }

    ui->Vprixttctxt_2->setText(QString::number(prix_ttc, 'f', 2));

    // Debug
    qDebug() << "💰 Calcul TTC: HT=" << prix_ht
             << "TVA=" << tva << "%"
             << "Remise=" << remise << "%"
             << "TTC=" << prix_ttc;
}
void OpticalStore::on_Vprixht_txt_2_textChanged(const QString &)
{
    calculerPrixTTC();
}

void OpticalStore::on_Vremisetxt_2_textChanged(const QString &)
{
    calculerPrixTTC();
}

// ===================== CRUD CLIENTS + PHOTOS =====================

// Vider champs client (on ne touche pas à la photo ici)
void OpticalStore::viderChampsClient()
{
    ui->idclient->clear();
    ui->nom->clear();
    ui->prenom->clear();
    ui->email->clear();
    ui->telephone->clear();
    ui->addresse->clear();
    ui->datenaissance->clear();
}

// Afficher tous les clients
void OpticalStore::afficherClients()
{
    qDebug() << "=== AFFICHER CLIENTS ===";

    QSqlQuery query;
    if (!query.exec("SELECT ID_CLIENT, NOM, PRENOM, EMAIL, TELEPHONE, ADRESSE,"
                    "DATE_NAISSANCE, FIDELITE FROM CLIENTS ORDER BY ID_CLIENT")) {
        qDebug() << "❌ Erreur lors de l'exécution de la requête:"
                 << query.lastError().text();
        QMessageBox::critical(this, "Erreur Base de Données",
                              "Erreur lors du chargement des clients:\n"
                                  + query.lastError().text());
        return;
    }

    ui->tablelist->setRowCount(0);
    int row = 0;

    while (query.next()) {
        ui->tablelist->insertRow(row);
        ui->tablelist->setItem(row, 0, new QTableWidgetItem(query.value(0).toString()));
        ui->tablelist->setItem(row, 1, new QTableWidgetItem(query.value(1).toString()));
        ui->tablelist->setItem(row, 2, new QTableWidgetItem(query.value(2).toString()));
        ui->tablelist->setItem(row, 3, new QTableWidgetItem(query.value(3).toString()));
        ui->tablelist->setItem(row, 4, new QTableWidgetItem(query.value(4).toString()));
        ui->tablelist->setItem(row, 5, new QTableWidgetItem(query.value(5).toString()));
        ui->tablelist->setItem(row, 6, new QTableWidgetItem(query.value(6).toString()));
        ui->tablelist->setItem(row, 7, new QTableWidgetItem(query.value(7).toString()));
        row++;
    }

    qDebug() << "✅ Nombre de clients affichés:" << row;
}

// Ajouter client + photo
void OpticalStore::on_addclient_clicked()
{
    if (ui->nom->text().isEmpty() || ui->prenom->text().isEmpty()) {
        QMessageBox::warning(this, "Champs manquants",
                             "Nom et Prénom sont obligatoires !");
        return;
    }

    if (ui->idclient->text().isEmpty()) {
        QMessageBox::warning(this, "Champ manquant",
                             "ID client est obligatoire !");
        return;
    }

    int id_client = ui->idclient->text().toInt();
    QString nom = ui->nom->text();
    QString prenom = ui->prenom->text();
    QString adresse = ui->addresse->text();
    QString date_naissance = ui->datenaissance->text();
    QString email = ui->email->text();
    QString telephone = ui->telephone->text();

    gc->setIdClient(id_client);
    gc->setNomClient(nom);
    gc->setPrenomClient(prenom);
    gc->setAdresseClient(adresse);
    gc->setDateNaissanceClient(date_naissance);
    gc->setEmailClient(email);
    gc->setTelephoneClient(telephone);
    gc->setFideliteClient(0);

    if (gc->ajouter()) {
        // Sauvegarder la photo après ajout
        sauvegarderPhotoClient(id_client);

        QMessageBox::information(this, "Succès",
                                 "Client ajouté avec succès !");
        afficherClients();
        viderChampsClient();

        photoActuelle.clear();
        viderPhotoInterface();
    } else {
        QMessageBox::critical(this, "Erreur",
                              "L'ajout du client a échoué !");
    }
}

// Modifier client + photo
void OpticalStore::on_edit_clicked()
{
    if (ui->idclient->text().isEmpty()) {
        QMessageBox::warning(this, "Champ manquant",
                             "Veuillez saisir un ID client à modifier !");
        return;
    }

    int id_client = ui->idclient->text().toInt();
    QString nom = ui->nom->text();
    QString prenom = ui->prenom->text();
    QString adresse = ui->addresse->text();
    QString date_naissance = ui->datenaissance->text();
    QString email = ui->email->text();
    QString telephone = ui->telephone->text();

    if (gc->modifier(id_client, nom, prenom, email, telephone, adresse,
                     date_naissance, 0)) {
        sauvegarderPhotoClient(id_client);

        QMessageBox::information(this, "Succès",
                                 "Client modifié avec succès !");
        afficherClients();
    } else {
        QMessageBox::critical(this, "Erreur",
                              "La modification du client a échoué !");
    }
}

// Clic sur un client => remplir champs + charger photo
void OpticalStore::on_tablelist_cellClicked(int row, int column)
{
    Q_UNUSED(column);

    int idClient = ui->tablelist->item(row, 0)->text().toInt();

    ui->idclient->setText(ui->tablelist->item(row, 0)->text());
    ui->nom->setText(ui->tablelist->item(row, 1)->text());
    ui->prenom->setText(ui->tablelist->item(row, 2)->text());
    ui->email->setText(ui->tablelist->item(row, 3)->text());
    ui->telephone->setText(ui->tablelist->item(row, 4)->text());
    ui->addresse->setText(ui->tablelist->item(row, 5)->text());
    ui->datenaissance->setText(ui->tablelist->item(row, 6)->text());
    QString pointsFidelite = ui->tablelist->item(row, 7)->text();


    // Charger la photo
    chargerPhotoClient(idClient);
}

// Supprimer client + photo UI
void OpticalStore::on_deleteclient_clicked()
{
    qDebug() << "=== INTERFACE: Début suppression ===";

    if (ui->idclient->text().isEmpty()) {
        QMessageBox::warning(this, "Champ manquant",
                             "Veuillez saisir un ID client à supprimer !");
        return;
    }

    bool ok;
    int id_client = ui->idclient->text().toInt(&ok);

    if (!ok || id_client <= 0) {
        QMessageBox::warning(this, "ID invalide",
                             "L'ID client doit être un nombre positif !");
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirmation",
        "Êtes-vous sûr de vouloir supprimer le client ID: "
            + QString::number(id_client) + "?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        qDebug() << "Suppression annulée par l'utilisateur";
        return;
    }

    qDebug() << "=== INTERFACE: Appel de gc->supprimer(" << id_client << ") ===";

    ui->deleteclient->setEnabled(false);
    bool success = gc->supprimer(id_client);
    ui->deleteclient->setEnabled(true);

    if (success) {
        QMessageBox::information(this, "Succès",
                                 "Client supprimé avec succès !");
        afficherClients();
        viderChampsClient();

        photoActuelle.clear();
        viderPhotoInterface();
    } else {
        QMessageBox::warning(this, "Avertissement",
                             "Échec de la suppression. Le client n'existe pas, a des achats associés ou une erreur s'est produite.");
    }

    qDebug() << "=== INTERFACE: Fin suppression ===";
}

// ==================== GESTION PHOTOS CLIENT ====================

void OpticalStore::on_importerPhoto_clicked()
{
    QString fichier = QFileDialog::getOpenFileName(
        this,
        "Sélectionner une photo",
        QDir::homePath(),
        "Images (*.png *.jpg *.jpeg *.bmp *.gif)");

    if (!fichier.isEmpty()) {
        QFile file(fichier);
        if (file.open(QIODevice::ReadOnly)) {
            photoActuelle = file.readAll();
            file.close();

            afficherPhoto(photoActuelle);

            QFileInfo fileInfo(fichier);
            ui->statusbar->showMessage(
                QString("Photo chargée: %1 (%2 KB)")
                    .arg(fileInfo.fileName())
                    .arg(photoActuelle.size() / 1024)
                );
        } else {
            QMessageBox::warning(this, "Erreur",
                                 "Impossible de charger la photo.");
        }
    }
}

void OpticalStore::on_supprimerPhoto_clicked()
{
    photoActuelle.clear();
    viderPhotoInterface();
    ui->statusbar->showMessage("Photo supprimée");
}

void OpticalStore::afficherPhoto(const QByteArray &photoData)
{
    if (photoData.isEmpty()) {
        viderPhotoInterface();
        return;
    }

    QPixmap pixmap;
    if (pixmap.loadFromData(photoData)) {
        QPixmap scaledPixmap = pixmap.scaled(
            ui->labelPhoto->width(),
            ui->labelPhoto->height(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation);
        ui->labelPhoto->setPixmap(scaledPixmap);
        ui->labelPhoto->setStyleSheet(
            "QLabel { border: 2px solid #4CAF50; background-color: white; }");
    } else {
        QMessageBox::warning(this, "Erreur", "Format d'image non supporté.");
        viderPhotoInterface();
    }
}

void OpticalStore::viderPhotoInterface()
{
    ui->labelPhoto->setPixmap(QPixmap());
    ui->labelPhoto->setText("Aucune photo\n(150x150 px)");
    ui->labelPhoto->setAlignment(Qt::AlignCenter);
    ui->labelPhoto->setStyleSheet(
        "QLabel { border: 2px dashed #ccc; background-color: #f9f9f9; color: #999; }");
}

void OpticalStore::sauvegarderPhotoClient(int idClient)
{
    if (photoActuelle.isEmpty()) {
        qDebug() << "❌ Aucune photo à sauvegarder pour client ID:" << idClient;
        return;
    }

    QSqlQuery query;
    query.prepare("UPDATE CLIENTS SET PHOTO = :photo WHERE ID_CLIENT = :id");
    query.bindValue(":photo", photoActuelle);
    query.bindValue(":id", idClient);

    if (query.exec()) {
        qDebug() << "✅ Photo sauvegardée pour client ID:" << idClient
                 << "Taille:" << photoActuelle.size() << "bytes";
        ui->statusbar->showMessage("Photo sauvegardée dans la base de données");
    } else {
        qDebug() << "❌ Erreur sauvegarde photo:"
                 << query.lastError().text();
        QMessageBox::warning(this, "Erreur",
                             "Erreur lors de la sauvegarde de la photo: "
                                 + query.lastError().text());
    }
}

void OpticalStore::chargerPhotoClient(int idClient)
{
    QSqlQuery query;
    query.prepare("SELECT PHOTO FROM CLIENTS WHERE ID_CLIENT = :id");
    query.bindValue(":id", idClient);

    if (query.exec() && query.next()) {
        QByteArray photoData = query.value(0).toByteArray();
        if (!photoData.isEmpty()) {
            photoActuelle = photoData;
            afficherPhoto(photoActuelle);
            qDebug() << "✅ Photo chargée depuis BDD pour client ID:" << idClient;
        } else {
            photoActuelle.clear();
            viderPhotoInterface();
            qDebug() << "ℹ️ Aucune photo enregistrée pour client ID:" << idClient;
        }
    } else {
        photoActuelle.clear();
        viderPhotoInterface();
        qDebug() << "❌ Erreur chargement photo client ID:"
                 << idClient << query.lastError().text();
    }
}

// ==================== RECHERCHE / TRI CLIENTS ====================

void OpticalStore::on_rechercher_2_clicked()
{
    QString nom = QInputDialog::getText(
        this,
        "Rechercher un client",
        "Entrez le nom ou prénom du client:");

    if (nom.isEmpty()) {
        return;
    }

    QSqlQueryModel* model = gc->rechercherParNom(nom);

    ui->tablelist->setRowCount(0);

    for (int row = 0; row < model->rowCount(); ++row) {
        ui->tablelist->insertRow(row);
        for (int col = 0; col < model->columnCount(); ++col) {
            QTableWidgetItem* item = new QTableWidgetItem(
                model->data(model->index(row, col)).toString());
            ui->tablelist->setItem(row, col, item);
        }
    }

    if (model->rowCount() == 0) {
        QMessageBox::information(this, "Recherche",
                                 "Aucun client trouvé avec le nom: " + nom);
    } else {
        QMessageBox::information(this, "Recherche",
                                 QString("%1 client(s) trouvé(s)")
                                     .arg(model->rowCount()));
    }

    delete model;
}

void OpticalStore::on_trier_clicked()
{
    QStringList options;
    options << "Ordre alphabétique (A-Z)"
            << "Ordre alphabétique (Z-A)"
            << "ID croissant"
            << "ID décroissant";

    bool ok;
    QString choix = QInputDialog::getItem(
        this,
        "Trier les clients",
        "Choisissez le type de tri:",
        options,
        0,
        false,
        &ok);

    if (!ok) return;

    QSqlQueryModel* model = nullptr;

    if (choix == "Ordre alphabétique (A-Z)") {
        model = gc->trierParOrdreAlphabetique(true);
    } else if (choix == "Ordre alphabétique (Z-A)") {
        model = gc->trierParOrdreAlphabetique(false);
    } else if (choix == "ID croissant") {
        model = gc->trierParID(true);
    } else if (choix == "ID décroissant") {
        model = gc->trierParID(false);
    }

    if (!model) {
        QMessageBox::warning(this, "Erreur",
                             "Erreur lors du tri!");
        return;
    }

    ui->tablelist->setRowCount(0);

    for (int row = 0; row < model->rowCount(); ++row) {
        ui->tablelist->insertRow(row);
        for (int col = 0; col < model->columnCount(); ++col) {
            QTableWidgetItem* item = new QTableWidgetItem(
                model->data(model->index(row, col)).toString());
            ui->tablelist->setItem(row, col, item);
        }
    }

    delete model;
    QMessageBox::information(this, "Tri",
                             "Clients triés par " + choix.toLower());
}

// ==================== STATISTIQUES CLIENTS (Camembert) ====================

void OpticalStore::on_statistiques_clicked()
{
    QMap<QString, int> stats = gc->statistiquesTranchesAge();

    int totalClients = 0;
    for (int count : stats) {
        totalClients += count;
    }

    if (totalClients == 0) {
        QMessageBox::information(
            this, "Statistiques",
            "📊 Aucun client avec date de naissance renseignée.\n\n"
            "💡 Pour avoir des statistiques complètes :\n"
            "• Ajoutez des dates de naissance aux clients existants\n"
            "• Ou créez de nouveaux clients avec date de naissance");
        return;
    }

    QDialog *chartDialog = new QDialog(this);
    chartDialog->setWindowTitle(
        "📊 Statistiques Clients par Tranche d'Âge - LumaVision");
    chartDialog->setFixedSize(1000, 750);

    QVBoxLayout *layout = new QVBoxLayout(chartDialog);

    QPieSeries *series = new QPieSeries();

    QVector<QColor> colors = {
        QColor(65, 105, 225),
        QColor(220, 20, 60),
        QColor(46, 139, 87),
        QColor(255, 140, 0),
        QColor(148, 0, 211)
    };

    int colorIndex = 0;

    for (auto it = stats.begin(); it != stats.end(); ++it) {
        QString tranche = it.key();
        int count = it.value();

        if (count > 0) {
            double pourcentage = (count * 100.0) / totalClients;
            QString label = QString("%1\n%2 clients (%3%)")
                                .arg(tranche)
                                .arg(count)
                                .arg(QString::number(pourcentage, 'f', 1));

            QPieSlice *slice = series->append(label, count);
            slice->setColor(colors[colorIndex % colors.size()]);
            slice->setLabelVisible(true);
            slice->setLabelPosition(QPieSlice::LabelOutside);
            slice->setLabelArmLengthFactor(0.1);
            slice->setBorderWidth(1);

            colorIndex++;
        }
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("📊 Répartition des Clients par Tranche d'Âge");
    chart->setTitleFont(QFont("Arial", 18, QFont::Bold));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->legend()->setFont(QFont("Arial", 10));
    chart->setBackgroundBrush(QBrush(QColor(248, 249, 250)));
    chart->setBackgroundVisible(true);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumSize(800, 500);
    chartView->setStyleSheet(
        "background-color: white; border-radius: 10px;");

    QLabel *infoLabel = new QLabel();
    infoLabel->setText(
        QString(
            "<div style='background-color: #2c3e50; color: white; padding: 20px; border-radius: 10px;'>"
            "<h2 style='margin: 0;'>📈 STATISTIQUES CLIENTS</h2>"
            "<p style='font-size: 16px; margin: 10px 0;'><b>👥 Clients analysés :</b> %1</p>"
            "<p style='font-size: 14px; margin: 5px 0; opacity: 0.9;'><i>Note : Seuls les clients avec date de naissance sont inclus</i></p>"
            "<p style='font-size: 14px; margin: 5px 0;'><b>📅 Généré le :</b> %2</p>"
            "</div>")
            .arg(totalClients)
            .arg(QDateTime::currentDateTime().toString("dd/MM/yyyy à HH:mm")));
    infoLabel->setWordWrap(true);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *closeButton = new QPushButton("🗙 Fermer");
    QPushButton *saveButton = new QPushButton("💾 Sauvegarder le diagramme");

    closeButton->setStyleSheet(
        "QPushButton { "
        "background-color: #e74c3c; "
        "color: white; "
        "padding: 12px 25px; "
        "border-radius: 8px; "
        "font-weight: bold; "
        "font-size: 14px; "
        "min-width: 120px; "
        "}");
    saveButton->setStyleSheet(
        "QPushButton { "
        "background-color: #3498db; "
        "color: white; "
        "padding: 12px 25px; "
        "border-radius: 8px; "
        "font-weight: bold; "
        "font-size: 14px; "
        "min-width: 120px; "
        "}");

    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(closeButton);

    layout->addWidget(infoLabel);
    layout->addSpacing(10);
    layout->addWidget(chartView);
    layout->addSpacing(10);
    layout->addLayout(buttonLayout);

    connect(closeButton, &QPushButton::clicked,
            chartDialog, &QDialog::accept);

    connect(saveButton, &QPushButton::clicked, [chart, this]() {
        QString fileName = QFileDialog::getSaveFileName(
            this,
            "Sauvegarder le diagramme",
            QDir::homePath() + "/statistiques_clients_lumavision.png",
            "Images (*.png *.jpg *.bmp)"
            );

        if (!fileName.isEmpty()) {
            QPixmap pixmap = chart->scene()->views().first()->grab();
            pixmap = pixmap.scaled(1200, 800,
                                   Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);

            if (pixmap.save(fileName)) {
                QMessageBox::information(
                    this, "✅ Succès",
                    "Diagramme sauvegardé avec succès !\n\n"
                    "📁 Fichier : " + QFileInfo(fileName).fileName() + "\n"
                                                           "📊 Taille : " + QString::number(pixmap.width()) + "x"
                        + QString::number(pixmap.height()) + " pixels");
            } else {
                QMessageBox::warning(this, "❌ Erreur",
                                     "Erreur lors de la sauvegarde du diagramme.");
            }
        }
    });

    chartDialog->exec();
    delete chartDialog;
}

// ==================== AI : ANALYSE FORME DE VISAGE ====================

QString OpticalStore::analyzeFaceShape(const QByteArray &imageData)
{
    if (imageData.isEmpty()) {
        return "";
    }

    QUrl url("http://127.0.0.1:8000/predict");

    QNetworkAccessManager manager;
    QEventLoop loop;

    QObject::connect(&manager, &QNetworkAccessManager::finished,
                     &loop, &QEventLoop::quit);

    QNetworkRequest request(url);

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart imagePart;
    imagePart.setHeader(
        QNetworkRequest::ContentDispositionHeader,
        QVariant("form-data; name=\"file\"; filename=\"face.jpg\""));
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader,
                        QVariant("image/jpeg"));
    imagePart.setBody(imageData);

    multiPart->append(imagePart);

    QNetworkReply *reply = manager.post(request, multiPart);
    multiPart->setParent(reply);

    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(this, "API Error",
                             "Error: " + reply->errorString());
        reply->deleteLater();
        return "";
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonDocument json = QJsonDocument::fromJson(responseData);
    if (!json.isObject()) return "";

    QString shape = json["face_shape"].toString().trimmed().toLower();

    return shape;
}

// Recommandation lunettes
void OpticalStore::on_recommand_glass_btn_clicked()
{
    if (photoActuelle.isEmpty()) {
        QMessageBox::warning(this, "Erreur",
                             "Aucune photo pour ce client !");
        return;
    }

    QString faceType = analyzeFaceShape(photoActuelle);

    if (faceType.isEmpty()) {
        QMessageBox::warning(this, "Erreur",
                             "Impossible de détecter le type de visage.");
        return;
    }

    QString recommandation;

    if (faceType == "oval")        recommandation = "Rectangulaire ou carré";
    else if (faceType == "round")  recommandation = "Montures angulaires";
    else if (faceType == "square") recommandation = "Montures arrondies";
    else if (faceType == "heart")  recommandation = "Montures ovales";
    else if (faceType == "diamond")recommandation = "Montures fines et ovales";
    else                           recommandation = "Type de visage inconnu";

    QMessageBox::information(
        this,
        "Recommandation Lunettes",
        "Type de visage détecté : " + faceType +
            "\n\nRecommandation monture : " + recommandation);
}

// ==================== EXPORT PDF CLIENTS ====================

void OpticalStore::on_exporter_clicked()
{
    if (ui->tablelist->rowCount() == 0) {
        QMessageBox::information(this, "Export PDF",
                                 "Aucun client à exporter.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Exporter les clients en PDF",
        QDir::homePath() + "/clients_lumavision.pdf",
        "PDF (*.pdf)"
        );

    if (fileName.isEmpty()) return;

    try {
        QString html;
        html += "<!DOCTYPE html>";
        html += "<html>";
        html += "<head>";
        html += "<meta charset='UTF-8'>";
        html += "<style>";
        html += "body { font-family: Arial, sans-serif; margin: 40px; background: white; }";
        html += "h1 { color: #2c3e50; text-align: center; border-bottom: 2px solid #3498db; padding-bottom: 10px; }";
        html += ".info { text-align: center; color: #7f8c8d; margin: 10px 0; font-size: 14px; }";
        html += "table { width: 100%; border-collapse: collapse; margin: 25px 0; font-size: 12px; }";
        html += "th { background: #3498db; color: white; padding: 8px; text-align: left; border: 1px solid #ddd; }";
        html += "td { padding: 6px; border: 1px solid #ddd; }";
        html += "tr:nth-child(even) { background: #f8f9fa; }";
        html += ".footer { margin-top: 40px; text-align: right; color: #95a5a6; font-size: 10px; border-top: 1px solid #eee; padding-top: 10px; }";
        html += "</style>";
        html += "</head>";
        html += "<body>";

        html += "<h1>📋 Liste des Clients - LumaVision</h1>";
        html += QString("<div class='info'><strong>Date d'export :</strong> %1</div>")
                    .arg(QDateTime::currentDateTime().toString("dd/MM/yyyy à HH:mm"));
        html += QString("<div class='info'><strong>Nombre total de clients :</strong> %1</div>")
                    .arg(ui->tablelist->rowCount());

        html += "<table>";
        html += "<thead><tr>";

        QStringList headers;
        headers << "ID" << "Nom" << "Prénom" << "Email"
                << "Téléphone" << "Adresse"
                << "Date de naissance" << "Fidélité";

        for (const QString &header : headers) {
            html += QString("<th>%1</th>").arg(header);
        }
        html += "</tr></thead>";
        html += "<tbody>";

        for (int row = 0; row < ui->tablelist->rowCount(); ++row) {
            html += "<tr>";
            for (int col = 0; col < ui->tablelist->columnCount(); ++col) {
                QTableWidgetItem *item = ui->tablelist->item(row, col);
                QString cellText = item ? item->text() : "";
                html += QString("<td>%1</td>")
                            .arg(cellText.toHtmlEscaped());
            }
            html += "</tr>";
        }

        html += "</tbody>";
        html += "</table>";

        html += "<div class='footer'>";
        html += "Exporté par LumaVision - Système de gestion optique";
        html += "</div>";

        html += "</body>";
        html += "</html>";

        QPrinter printer;
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(fileName);
        printer.setPageSize(QPageSize(QPageSize::A4));
        printer.setPageMargins(QMarginsF(10, 10, 10, 10),
                               QPageLayout::Millimeter);

        QTextDocument document;
        document.setHtml(html);
        document.print(&printer);

        QMessageBox::information(
            this, "Export PDF",
            QString("PDF exporté avec succès !\nFichier : %1")
                .arg(QFileInfo(fileName).fileName()));
    }
    catch (const std::exception& e) {
        QMessageBox::critical(this, "Erreur",
                              QString("Erreur lors de l'exportation PDF : %1")
                                  .arg(e.what()));
    }
    catch (...) {
        QMessageBox::critical(this, "Erreur",
                              "Une erreur inconnue est survenue lors de l'exportation du PDF.");
    }
}

// ==================== NAVIGATION STACKED WIDGET ====================

void OpticalStore::on_gestionemp_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_3);
}

void OpticalStore::on_quitcon_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page);
}

void OpticalStore::on_QuitterEMP_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_2);
}

void OpticalStore::on_quiteclients_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_2);
}

void OpticalStore::on_gclient_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_4);
}

void OpticalStore::on_quitterlog_clicked()
{
    this->close();
}

void OpticalStore::on_accueilfourni_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->fournisseur);
}

void OpticalStore::on_retourhome_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_2);
}

void OpticalStore::on_retourhome_2_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_2);
}

void OpticalStore::on_GestionV_bt_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_5);
}

void OpticalStore::on_quitterlog_2_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page);
}

void OpticalStore::on_pushButton_7_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_6);
}

void OpticalStore::on_connectlog_2_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_2);
}

void OpticalStore::on_btnRetour_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_2);
}

void OpticalStore::on_btnGestionStock_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_7);
}

void OpticalStore::on_return_4_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page);
}

void OpticalStore::on_return_2_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page);
}
void TickDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    // Dessiner un tick vert si la ligne est sélectionnée
    if (option.state & QStyle::State_Selected) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QPen(Qt::darkGreen, 3));

        QRect rect = option.rect;
        int size = qMin(rect.width(), rect.height()) / 3;
        QPoint center = rect.center();

        // Dessiner un checkmark (✓)
        painter->drawLine(center.x() - size/2, center.y(),
                          center.x() - size/6, center.y() + size/2);
        painter->drawLine(center.x() - size/6, center.y() + size/2,
                          center.x() + size/2, center.y() - size/2);

        painter->restore();
    }
}
void OpticalStore::afficherDetailsVentes()
{
    QSqlQueryModel* model = new QSqlQueryModel();
    model->setQuery("SELECT ID_VENTE, TO_CHAR(DATE_VENTE, 'DD/MM/YYYY') as DATE_VENTE, PRIX_TTC_VENTE FROM VENTES ORDER BY ID_VENTE");

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID Vente"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Date"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Prix TTC"));

    ui->dtableView1->setModel(model);
    ui->dtableView1->resizeColumnsToContents();

    // Ajustement des colonnes
    ui->dtableView1->setColumnWidth(0, 80);
    ui->dtableView1->setColumnWidth(1, 100);
    ui->dtableView1->setColumnWidth(2, 100);

    // Formater les prix
    for (int row = 0; row < model->rowCount(); ++row) {
        QModelIndex prixIndex = model->index(row, 2);
        if (prixIndex.isValid()) {
            double prix = model->data(prixIndex).toDouble();
            model->setData(prixIndex, QString::number(prix, 'f', 2) + " €");
        }
    }
}

void OpticalStore::afficherDetailsProduits()
{
    QSqlQueryModel* model = new QSqlQueryModel();
    model->setQuery("SELECT ID_PRODUIT, NOM_PRODUIT, CATEGORIE, PRIX_UNITAIRE, QUANTITE FROM PRODUIT ORDER BY ID_PRODUIT");

    model->setHeaderData(0, Qt::Horizontal, QObject::tr("ID Produit"));
    model->setHeaderData(1, Qt::Horizontal, QObject::tr("Nom"));
    model->setHeaderData(2, Qt::Horizontal, QObject::tr("Catégorie"));
    model->setHeaderData(3, Qt::Horizontal, QObject::tr("Prix"));
    model->setHeaderData(4, Qt::Horizontal, QObject::tr("Stock"));

    ui->dtableView2->setModel(model);
    ui->dtableView2->resizeColumnsToContents();

    // Ajustement des colonnes
    ui->dtableView2->setColumnWidth(0, 80);
    ui->dtableView2->setColumnWidth(1, 150);
    ui->dtableView2->setColumnWidth(2, 120);
    ui->dtableView2->setColumnWidth(3, 80);
    ui->dtableView2->setColumnWidth(4, 60);

    // Formater les prix
    for (int row = 0; row < model->rowCount(); ++row) {
        QModelIndex prixIndex = model->index(row, 3);
        if (prixIndex.isValid()) {
            double prix = model->data(prixIndex).toDouble();
            model->setData(prixIndex, QString::number(prix, 'f', 2) + " €");
        }
    }
}

void OpticalStore::afficherDetailsContenir()
{
    qDebug() << "=== DÉBUT afficherDetailsContenir ===";

    // Désélectionner avant de rafraîchir
    ui->dtableView3->clearSelection();
    ui->modifierContenir->setEnabled(false);
    ui->supprimerContenir->setEnabled(false);

    if (gv == nullptr) {
        qDebug() << "❌ ERREUR: gv est null!";
        return;
    }

    // Charger via GestionVentes
    QSqlQueryModel* model = gv->afficherDetailsVente();
    if (model) {
        qDebug() << "✅ Modèle CONTENIR chargé - Lignes:" << model->rowCount();
        ui->dtableView3->setModel(model);

        // Ajustement intelligent des colonnes
        ui->dtableView3->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);

        // Définir des largeurs spécifiques
        if (model->columnCount() >= 8) {
            ui->dtableView3->setColumnWidth(0, 80);  // ID Vente
            ui->dtableView3->setColumnWidth(1, 100); // Date
            ui->dtableView3->setColumnWidth(2, 80);  // ID Produit
            ui->dtableView3->setColumnWidth(3, 150); // Nom Produit
            ui->dtableView3->setColumnWidth(4, 120); // Catégorie
            ui->dtableView3->setColumnWidth(5, 60);  // Quantité
            ui->dtableView3->setColumnWidth(6, 100); // Prix Unitaire
            ui->dtableView3->setColumnWidth(7, 100); // Sous-Total
        }

        // Formater les nombres
        for (int row = 0; row < model->rowCount(); ++row) {
            // Formater le prix unitaire
            QModelIndex prixIndex = model->index(row, 6);
            if (prixIndex.isValid()) {
                double prix = model->data(prixIndex).toDouble();
                model->setData(prixIndex, QString::number(prix, 'f', 2) + " €");
            }

            // Formater le sous-total
            QModelIndex totalIndex = model->index(row, 7);
            if (totalIndex.isValid()) {
                double total = model->data(totalIndex).toDouble();
                model->setData(totalIndex, QString::number(total, 'f', 2) + " €");
            }
        }

        // Forcer l'affichage
        ui->dtableView3->update();
        ui->dtableView3->repaint();

        qDebug() << "✅ Tableau CONTENIR affiché -" << model->rowCount() << "lignes";

    } else {
        qDebug() << "❌ Modèle CONTENIR null";
        // Créer un modèle vide en cas d'erreur
        QSqlQueryModel* emptyModel = new QSqlQueryModel();
        emptyModel->setQuery("SELECT '' as ID_VENTE, '' as DATE_VENTE, '' as ID_PRODUIT, "
                             "'' as NOM_PRODUIT, '' as CATEGORIE, '' as QUANTITE, "
                             "'' as PRIX_UNITAIRE, '' as SOUS_TOTAL FROM DUAL WHERE 1=0");
        ui->dtableView3->setModel(emptyModel);
    }

    // Mettre à jour les indicateurs après rafraîchissement
    mettreAJourIndicateurs();

    qDebug() << "=== FIN afficherDetailsContenir ===";
}

void OpticalStore::on_dtableView1_clicked(const QModelIndex &index)
{
    int row = index.row();
    selectedVenteId = index.sibling(row, 0).data().toInt();
    QString dateVente = index.sibling(row, 1).data().toString();

    qDebug() << "Vente sélectionnée:" << selectedVenteId << "Date:" << dateVente;

    // Mettre à jour l'indicateur
    mettreAJourIndicateurs();

    // Recalculer le total si les deux sélections sont faites
    if (selectedVenteId != -1 && selectedProduitId != -1) {
        calculerTotal();
    }
}

void OpticalStore::on_dtableView2_clicked(const QModelIndex &index)
{
    int row = index.row();
    selectedProduitId = index.sibling(row, 0).data().toInt();
    selectedProduitPrix = index.sibling(row, 3).data().toDouble();
    int stock = index.sibling(row, 4).data().toInt();
    QString nomProduit = index.sibling(row, 1).data().toString();

    qDebug() << "Produit sélectionné:" << selectedProduitId << "Nom:" << nomProduit << "Prix:" << selectedProduitPrix << "Stock:" << stock;

    // Mettre à jour l'indicateur
    mettreAJourIndicateurs();

    // Recalculer le total si les deux sélections sont faites
    if (selectedVenteId != -1 && selectedProduitId != -1) {
        calculerTotal();
    }
}

void OpticalStore::on_dtableView3_clicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        qDebug() << "❌ Index invalide dans dtableView3";
        // Désactiver les boutons si clic invalide
        ui->modifierContenir->setEnabled(false);
        ui->supprimerContenir->setEnabled(false);
        return;
    }

    int row = index.row();
    QAbstractItemModel* model = ui->dtableView3->model();

    // Version sécurisée : Utiliser le modèle directement
    int idVente = model->data(model->index(row, 0)).toInt();
    int idProduit = model->data(model->index(row, 2)).toInt();
    int quantite = model->data(model->index(row, 5)).toInt();
    double prixUnitaire = model->data(model->index(row, 6)).toDouble();
    QString nomProduit = model->data(model->index(row, 3)).toString();

    qDebug() << "📋 Ligne CONTENIR sélectionnée - Vente:" << idVente << "Produit:" << idProduit
             << "(" << nomProduit << ")" << "Quantité:" << quantite << "Prix:" << prixUnitaire;

    // Mettre en surbrillance les lignes correspondantes dans les autres tableaux
    surlignerLignesCorrespondantes(idVente, idProduit);

    // Afficher les informations dans les champs
    ui->quantiteDetail->setText(QString::number(quantite));
    calculerTotal();

    // Activer les boutons maintenant qu'une ligne est sélectionnée
    ui->modifierContenir->setEnabled(true);
    ui->supprimerContenir->setEnabled(true);

    // Mettre à jour l'indicateur
    mettreAJourIndicateurs();

    qDebug() << "🔄 Quantité mise à jour:" << quantite;
}

void OpticalStore::calculerTotal()
{
    if (selectedVenteId == -1 || selectedProduitId == -1) {
        ui->totalDetail->clear();
        return;
    }

    if (ui->quantiteDetail->text().isEmpty()) {
        ui->totalDetail->clear();
        return;
    }

    int quantite = ui->quantiteDetail->text().toInt();

    if (quantite <= 0) {
        ui->totalDetail->clear();
        return;
    }

    double total = quantite * selectedProduitPrix;
    ui->totalDetail->setText(QString::number(total, 'f', 2) + " €");
}
void OpticalStore::on_quantiteDetail_textChanged(const QString &text)
{
    Q_UNUSED(text);
    calculerTotal();
}
void OpticalStore::mettreAJourIndicateurs()
{
    // Vérifier si une ligne CONTENIR est sélectionnée
    QModelIndexList selectedContenir = ui->dtableView3->selectionModel()->selectedRows();
    bool ligneContenirSelectionnee = !selectedContenir.isEmpty();

    QString indicateurVente = (selectedVenteId != -1) ?
                                  QString("✅ Vente sélectionnée: %1").arg(selectedVenteId) :
                                  "❌ Aucune vente sélectionnée";

    QString indicateurProduit = (selectedProduitId != -1) ?
                                    QString("✅ Produit sélectionné: %1").arg(selectedProduitId) :
                                    "❌ Aucun produit sélectionné";

    QString indicateurContenir = ligneContenirSelectionnee ?
                                     QString("📋 Ligne CONTENIR sélectionnée - Prête pour modification") :
                                     "❌ Aucune ligne CONTENIR sélectionnée";

    // Mettre à jour les tooltips avec plus d'informations
    ui->dtableView1->setToolTip(indicateurVente + "\nCliquez pour sélectionner une vente");
    ui->dtableView2->setToolTip(indicateurProduit + "\nCliquez pour sélectionner un produit");
    ui->dtableView3->setToolTip(indicateurContenir + "\n✓ Ligne sélectionnée");

    // Gérer l'état des boutons
    bool quantiteSaisie = !ui->quantiteDetail->text().isEmpty();
    bool peutModifier = ligneContenirSelectionnee && quantiteSaisie;

    ui->modifierContenir->setEnabled(peutModifier);
    ui->supprimerContenir->setEnabled(ligneContenirSelectionnee);

    // Mettre à jour le statut dans l'interface
    if (ligneContenirSelectionnee) {
        if (quantiteSaisie) {
            ui->quantiteDetail->setPlaceholderText("Quantité modifiée - Cliquez 'Modifier' pour confirmer");
        } else {
            ui->quantiteDetail->setPlaceholderText("Saisir la nouvelle quantité puis cliquez 'Modifier'");
        }
    } else {
        ui->quantiteDetail->setPlaceholderText("Sélectionnez d'abord une ligne CONTENIR");
        // Désactiver les boutons si aucune sélection
        ui->modifierContenir->setEnabled(false);
        ui->supprimerContenir->setEnabled(false);
    }

    qDebug() << "📊" << indicateurVente << "-" << indicateurProduit << "-" << indicateurContenir;
}

void OpticalStore::surlignerLignesCorrespondantes(int idVente, int idProduit)
{
    qDebug() << "🔍 Surlignage des lignes correspondantes - Vente:" << idVente << "Produit:" << idProduit;

    // Surligner la vente correspondante dans dtableView1
    QAbstractItemModel* modelVentes = ui->dtableView1->model();
    if (modelVentes) {
        bool venteTrouvee = false;
        for (int row = 0; row < modelVentes->rowCount(); ++row) {
            QModelIndex index = modelVentes->index(row, 0); // Colonne 0 = ID Vente
            if (index.isValid() && index.data().toInt() == idVente) {
                ui->dtableView1->selectRow(row);
                venteTrouvee = true;
                qDebug() << "✅ Vente trouvée et sélectionnée à la ligne:" << row;
                break;
            }
        }
        if (!venteTrouvee) {
            qDebug() << "❌ Vente non trouvée dans le tableau";
            ui->dtableView1->clearSelection();
        }
    }

    // Surligner le produit correspondant dans dtableView2
    QAbstractItemModel* modelProduits = ui->dtableView2->model();
    if (modelProduits) {
        bool produitTrouve = false;
        for (int row = 0; row < modelProduits->rowCount(); ++row) {
            QModelIndex index = modelProduits->index(row, 0); // Colonne 0 = ID Produit
            if (index.isValid() && index.data().toInt() == idProduit) {
                ui->dtableView2->selectRow(row);
                produitTrouve = true;

                // Mettre à jour le prix du produit sélectionné
                QModelIndex indexPrix = modelProduits->index(row, 3); // Colonne 3 = Prix
                if (indexPrix.isValid()) {
                    selectedProduitPrix = indexPrix.data().toDouble();
                }

                qDebug() << "✅ Produit trouvé et sélectionné à la ligne:" << row << "Prix:" << selectedProduitPrix;
                break;
            }
        }
        if (!produitTrouve) {
            qDebug() << "❌ Produit non trouvé dans le tableau";
            ui->dtableView2->clearSelection();
        }
    }

    // Mettre à jour les variables de sélection
    selectedVenteId = idVente;
    selectedProduitId = idProduit;

    // Mettre à jour l'indicateur
    mettreAJourIndicateurs();

    qDebug() << "🎯 Sélections mises à jour - Vente:" << selectedVenteId << "Produit:" << selectedProduitId;
}

void OpticalStore::appliquerStyleSelection()
{
    // STYLE AVEC VOS COULEURS EXISTANTES
    QString styleCommun =
        "QTableView {"
        "    background-color: #f5f5f5;"                   /* Light gray background - VOTRE COULEUR */
        "    color: black;"                                /* Text color - VOTRE COULEUR */
        "    gridline-color: #d0d0d0;"                     /* Subtle grid - VOTRE COULEUR */
        "    border: 1px solid #c0c0c0;"
        "    border-radius: 4px;"
        "}"
        "QTableView::item {"
        "    border-bottom: 1px solid #e0e0e0;"
        "    padding: 6px;"
        "}"
        "QTableView::item:selected {"
        "    background-color: #ffd966;"                   /* Yellow highlight - VOTRE COULEUR */
        "    color: black;"                                /* VOTRE COULEUR */
        "    border: none;"
        "    font-weight: bold;"
        "}"
        "QTableView::item:hover {"
        "    background-color: #e8e8e8;"
        "}"
        "QHeaderView::section {"
        "    background-color: #4b2e83;"                   /* Purple header - VOTRE COULEUR */
        "    color: white;"                                /* White header text - VOTRE COULEUR */
        "    font-weight: bold;"
        "    padding: 10px 8px;"
        "    border: none;"
        "    border-right: 1px solid #5a3a9c;"
        "    font-size: 11px;"
        "}"
        "QHeaderView::section:last {"
        "    border-right: none;"
        "}"
        "QHeaderView::section:hover {"
        "    background-color: #5a3a9c;"
        "}"
        "QTableView QTableCornerButton::section {"
        "    background-color: #4b2e83;"
        "    border: none;"
        "}"
        "/* Scrollbar discrète */"
        "QScrollBar:vertical {"
        "    background-color: #f0f0f0;"
        "    width: 10px;"
        "    margin: 0px;"
        "    border-radius: 5px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background-color: #c0c0c0;"
        "    border-radius: 5px;"
        "    min-height: 20px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background-color: #a0a0a0;"
        "}"
        "QScrollBar:horizontal {"
        "    background-color: #f0f0f0;"
        "    height: 10px;"
        "    margin: 0px;"
        "    border-radius: 5px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "    background-color: #c0c0c0;"
        "    border-radius: 5px;"
        "    min-width: 20px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "    background-color: #a0a0a0;"
        "}";

    // Appliquer le style commun à tous les tableaux
    ui->dtableView1->setStyleSheet(styleCommun);
    ui->dtableView2->setStyleSheet(styleCommun);
    ui->dtableView3->setStyleSheet(styleCommun);

    // AUGMENTER LA HAUTEUR DES LIGNES POUR MIEUX VOIR LES TICKS
    ui->dtableView1->verticalHeader()->setDefaultSectionSize(38);
    ui->dtableView2->verticalHeader()->setDefaultSectionSize(38);
    ui->dtableView3->verticalHeader()->setDefaultSectionSize(38);

    // MASQUER LES NUMÉROS DE LIGNE VERTICAUX POUR PLUS DE PROPRETÉ
    ui->dtableView1->verticalHeader()->setVisible(false);
    ui->dtableView2->verticalHeader()->setVisible(false);
    ui->dtableView3->verticalHeader()->setVisible(false);

    // POLICE LÉGÈREMENT AMÉLIORÉE
    QFont tableFont("Segoe UI", 9);
    ui->dtableView1->setFont(tableFont);
    ui->dtableView2->setFont(tableFont);
    ui->dtableView3->setFont(tableFont);

    // S'ASSURER QUE LA SÉLECTION EST BIEN CONFIGURÉE
    ui->dtableView1->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->dtableView1->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->dtableView2->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->dtableView2->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->dtableView3->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->dtableView3->setSelectionMode(QAbstractItemView::SingleSelection);

    qDebug() << "✅ Style des tableaux appliqué avec les couleurs existantes";
}

void OpticalStore::on_confirmerDetail_clicked()
{
    // Vérifications
    if (selectedVenteId == -1) {
        QMessageBox::warning(this, "Sélection manquante", "Veuillez sélectionner une vente dans le tableau de gauche!");
        return;
    }

    if (selectedProduitId == -1) {
        QMessageBox::warning(this, "Sélection manquante", "Veuillez sélectionner un produit dans le tableau de droite!");
        return;
    }

    if (ui->quantiteDetail->text().isEmpty()) {
        QMessageBox::warning(this, "Quantité manquante", "Veuillez saisir une quantité!");
        return;
    }

    int quantite = ui->quantiteDetail->text().toInt();

    if (quantite <= 0) {
        QMessageBox::warning(this, "Quantité invalide", "La quantité doit être positive!");
        return;
    }

    // Vérifier si l'association existe déjà
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM CONTENIR WHERE ID_VENTE = :id_vente AND ID_PRODUIT = :id_produit");
    checkQuery.bindValue(":id_vente", selectedVenteId);
    checkQuery.bindValue(":id_produit", selectedProduitId);

    if (checkQuery.exec() && checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        QMessageBox::warning(this, "Association existante",
                             "Cette association vente-produit existe déjà!\n"
                             "Veuillez sélectionner une autre combinaison.");
        return;
    }

    // Vérifier le stock disponible
    QSqlQuery stockQuery;
    stockQuery.prepare("SELECT QUANTITE_PRODUIT FROM PRODUIT WHERE ID_PRODUIT = :id_produit");
    stockQuery.bindValue(":id_produit", selectedProduitId);

    if (stockQuery.exec() && stockQuery.next()) {
        int stockDisponible = stockQuery.value(0).toInt();
        if (quantite > stockDisponible) {
            QMessageBox::warning(this, "Stock insuffisant",
                                 QString("Stock disponible: %1\nQuantité demandée: %2")
                                     .arg(stockDisponible).arg(quantite));
            return;
        }
    }

    // Confirmation
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirmation",
                                  QString("Confirmer l'ajout :\n"
                                          "Vente: %1\n"
                                          "Produit: %2\n"
                                          "Quantité: %3\n"
                                          "Total: %4 €")
                                      .arg(selectedVenteId)
                                      .arg(selectedProduitId)
                                      .arg(quantite)
                                      .arg(quantite * selectedProduitPrix, 0, 'f', 2),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

    // Enregistrement dans la table CONTENIR - Utiliser la nouvelle méthode de GestionVentes
    bool success = gv->ajouterDetailVenteAvecPrix(selectedVenteId, selectedProduitId, quantite);

    if (success) {
        QMessageBox::information(this, "Succès", "Détail de vente enregistré avec succès dans CONTENIR!");

        // Mettre à jour le stock (déjà fait dans la méthode ajouterDetailVenteAvecPrix)

        // Réinitialiser l'interface
        ui->quantiteDetail->clear();
        ui->totalDetail->clear();
        selectedVenteId = -1;
        selectedProduitId = -1;
        selectedProduitPrix = 0;

        // Désélectionner les lignes
        ui->dtableView1->clearSelection();
        ui->dtableView2->clearSelection();
        ui->dtableView3->clearSelection();

        // Rafraîchir les affichages
        afficherDetailsProduits(); // Rafraîchir l'affichage des produits (stock mis à jour)
        afficherDetailsContenir(); // Rafraîchir le tableau CONTENIR

        // Mettre à jour les indicateurs
        mettreAJourIndicateurs();

        qDebug() << "✅ Détail de vente enregistré dans CONTENIR - Vente:" << selectedVenteId << "Produit:" << selectedProduitId;

    } else {
        QMessageBox::critical(this, "Erreur", "Échec de l'enregistrement!");
    }
}

void OpticalStore::on_initialisationComplete()
{
    qDebug() << "⏰ Initialisation différée des données...";
    afficherDetailsContenir();
    mettreAJourIndicateurs();
}

// ===================== GESTION STATISTIQUES VENTES =====================

void OpticalStore::setupVentesCharts()
{
    if (!ui->groupBox_14) return;

    // Nettoyer complètement le layout
    if (QLayout* layout = ui->groupBox_14->layout()) {
        QLayoutItem* item;
        while ((item = layout->takeAt(0)) != nullptr) {
            if (QWidget* widget = item->widget()) {
                widget->setParent(nullptr);
                delete widget;
            }
            delete item;
        }
        delete layout;
    }

    // Créer un nouveau layout vide
    ui->groupBox_14->setLayout(new QVBoxLayout());
    ui->groupBox_14->layout()->setContentsMargins(0, 0, 0, 0);
    ui->groupBox_14->layout()->setSpacing(0);
}

void OpticalStore::updateVentesStats()
{
    qDebug() << "=== INITIALISATION DES STATISTIQUES ===";

    // Afficher le graphique initial
    updateCurrentChart();
}

void OpticalStore::on_stat_vente_clicked()
{
    qDebug() << "🔄 Bouton stat_vente cliqué!";
    switchGraphs();
}

void OpticalStore::switchGraphs()
{
    // Inverser l'état
    graphsSwitched = !graphsSwitched;
    qDebug() << "Nouvel état:" << (graphsSwitched ? "GRAPHE Prix" : "GRAPHE Mois");

    // Mettre à jour le graphique actuel
    updateCurrentChart();
}

void OpticalStore::updateCurrentChart()
{
    qDebug() << "=== MISE À JOUR DU GRAPHIQUE ===";

    if (!ui->groupBox_14) {
        qDebug() << "❌ groupBox_14 non trouvé";
        return;
    }

    // Nettoyer le layout
    setupVentesCharts();

    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(ui->groupBox_14->layout());
    if (!mainLayout) {
        mainLayout = new QVBoxLayout(ui->groupBox_14);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);
    }

    // Afficher le graphique selon l'état
    if (!graphsSwitched) {
        showVentesParMois();
    } else {
        showRepartitionPrix();
    }
}

void OpticalStore::showVentesParMois()
{
    qDebug() << "📊 Affichage: Ventes par Mois";

    // Récupérer les données
    QSqlQuery query;
    QMap<QString, int> ventesParMois;

    if (query.exec("SELECT DATE_VENTE FROM VENTES")) {
        while (query.next()) {
            QString dateStr = query.value(0).toString();
            QDate date;

            if (dateStr.contains('T')) {
                dateStr = dateStr.split('T').first();
                date = QDate::fromString(dateStr, "yyyy-MM-dd");
            } else if (dateStr.contains('/')) {
                date = QDate::fromString(dateStr, "dd/MM/yyyy");
            } else if (dateStr.contains('-')) {
                date = QDate::fromString(dateStr, "yyyy-MM-dd");
            }

            if (date.isValid()) {
                QString moisAbrege = date.toString("MMM");
                ventesParMois[moisAbrege]++;
            }
        }
    }

    // Créer le graphique
    QBarSeries *series = new QBarSeries();
    QBarSet *set = new QBarSet("Ventes");

    QStringList categories;
    for (auto it = ventesParMois.begin(); it != ventesParMois.end(); ++it) {
        *set << it.value();
        categories << it.key();
        qDebug() << "Mois:" << it.key() << "Ventes:" << it.value();
    }

    set->setColor(QColor(245, 194, 60)); // #F5C23C
    series->append(set);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("📈 Ventes par Mois");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundBrush(QColor(249, 249, 249));

    // Axes
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    axisX->setTitleText("Mois");
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setTitleText("Nombre de ventes");
    axisY->setLabelFormat("%d");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    chart->legend()->setVisible(false);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setFixedSize(680, 320);

    QFont titleFont = chart->titleFont();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    chart->setTitleFont(titleFont);

    // Ajouter au layout
    ui->groupBox_14->layout()->addWidget(chartView);

    qDebug() << "✅ Graphique Ventes par Mois affiché";
}

void OpticalStore::showRepartitionPrix()
{
    qDebug() << "🎯 Affichage: Répartition par Prix avec intervalles fixes";

    // Récupérer les données
    QSqlQuery query;
    QVector<double> prixVentes;

    if (query.exec("SELECT PRIX_TTC_VENTE FROM VENTES")) {
        while (query.next()) {
            double prixTTC = query.value(0).toDouble();
            prixVentes.append(prixTTC);
        }
    }

    if (!prixVentes.isEmpty()) {
        // Définir les intervalles fixes
        QMap<QString, int> intervallesPrix;

        // Intervalles spécifiques
        intervallesPrix["0-100€"] = 0;
        intervallesPrix["100-500€"] = 0;
        intervallesPrix["500-1000€"] = 0;
        intervallesPrix["1000-2000€"] = 0;
        intervallesPrix["2000€+"] = 0;

        // Compter les ventes dans chaque intervalle
        for (double prix : prixVentes) {
            if (prix < 100) {
                intervallesPrix["0-100€"]++;
            } else if (prix < 500) {
                intervallesPrix["100-500€"]++;
            } else if (prix < 1000) {
                intervallesPrix["500-1000€"]++;
            } else if (prix < 2000) {
                intervallesPrix["1000-2000€"]++;
            } else {
                intervallesPrix["2000€+"]++;
            }
        }

        // Créer le graphique secteurs
        QPieSeries *series = new QPieSeries();

        QList<QColor> sectorColors = {
            QColor(245, 194, 60),    // #F5C23C - Jaune
            QColor(46, 26, 98),      // #2E1A62 - Violet
            QColor(230, 180, 50),    // Jaune foncé
            QColor(65, 45, 115),     // Violet clair
            QColor(180, 140, 40)     // Jaune brun
        };

        int colorIndex = 0;
        int totalVentes = 0;

        // Calculer le total pour les pourcentages
        for (auto it = intervallesPrix.begin(); it != intervallesPrix.end(); ++it) {
            totalVentes += it.value();
        }

        for (auto it = intervallesPrix.begin(); it != intervallesPrix.end(); ++it) {
            if (it.value() > 0) {
                double pourcentage = (it.value() * 100.0) / totalVentes;
                QString label = QString("%1\n%2 ventes (%3%)")
                                    .arg(it.key())
                                    .arg(it.value())
                                    .arg(QString::number(pourcentage, 'f', 1));

                QPieSlice *slice = series->append(label, it.value());
                slice->setColor(sectorColors[colorIndex % sectorColors.size()]);
                slice->setLabelVisible(true);
                slice->setLabelBrush(QBrush(Qt::black));
                slice->setLabelPosition(QPieSlice::LabelOutside);

                colorIndex++;

                qDebug() << "Intervalle:" << it.key() << "Ventes:" << it.value() << "Pourcentage:" << pourcentage << "%";
            }
        }

        QChart *chart = new QChart();
        chart->addSeries(series);
        chart->setTitle("🎯 Répartition des Ventes par Prix");
        chart->setAnimationOptions(QChart::SeriesAnimations);
        chart->setBackgroundBrush(QColor(249, 249, 249));
        chart->legend()->setVisible(true);
        chart->legend()->setAlignment(Qt::AlignRight);

        // Personnaliser le titre
        QFont titleFont = chart->titleFont();
        titleFont.setPointSize(12);
        titleFont.setBold(true);
        chart->setTitleFont(titleFont);

        QChartView *chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->setFixedSize(680, 320);

        // Ajouter au layout
        ui->groupBox_14->layout()->addWidget(chartView);

        qDebug() << "✅ Graphique Répartition par Prix avec intervalles fixes affiché";
        qDebug() << "📊 Total des ventes analysées:" << totalVentes;

    } else {
        // Message si pas de données
        QLabel *noDataLabel = new QLabel("Aucune donnée de vente disponible");
        noDataLabel->setAlignment(Qt::AlignCenter);
        noDataLabel->setStyleSheet("color: #7f8c8d; font-size: 16px; padding: 100px;");
        noDataLabel->setFixedSize(680, 320);
        ui->groupBox_14->layout()->addWidget(noDataLabel);

        qDebug() << "❌ Aucune donnée de vente disponible";
    }
}

// ===================== FONCTIONS DE RECHERCHE ET TRI =====================

void OpticalStore::on_VExporterbt_2_clicked()
{
    // Vérifier rapidement s'il y a des ventes
    QSqlQuery checkQuery("SELECT COUNT(*) FROM VENTES");
    if (!checkQuery.next() || checkQuery.value(0).toInt() == 0) {
        QMessageBox::information(this, "Exportation", "Aucune vente à exporter dans la base de données.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Exporter les ventes en PDF",
        "Ventes_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmm") + ".pdf",
        "Fichiers PDF (*.pdf)"
        );

    if (fileName.isEmpty()) {
        return;
    }

    // Assurer l'extension .pdf
    if (!fileName.endsWith(".pdf", Qt::CaseInsensitive)) {
        fileName += ".pdf";
    }

    // Exporter directement sans messages intermédiaires
    bool success = gv->exporterPDF(fileName);

    if (success) {
        QMessageBox::information(this, "Exportation réussie",
                                 "✅ Les ventes ont été exportées en PDF avec succès !");
    } else {
        QMessageBox::warning(this, "Exportation échouée",
                             "❌ L'exportation a échoué. Vérifiez les données.");
    }
}

void OpticalStore::on_VtriComboBox_activated(int index)
{
    qDebug() << "🔄 Tri des ventes activé - Index:" << index << "Texte:" << ui->VtriComboBox->currentText();

    // Si index est -1 (pas de sélection), sortir
    if (index < 0) {
        qDebug() << "❌ Index invalide pour le tri";
        return;
    }

    // Récupérer le texte de l'option sélectionnée
    QString triOption = ui->VtriComboBox->currentText().trimmed();

    qDebug() << "Option de tri sélectionnée:" << triOption;

    // Déterminer l'ordre de tri
    Qt::SortOrder order = Qt::AscendingOrder;
    QString orderText = "croissant";

    if (triOption.contains("décroissant", Qt::CaseInsensitive) ||
        triOption.contains("desc", Qt::CaseInsensitive)) {
        order = Qt::DescendingOrder;
        orderText = "décroissant";
    }

    // Recharger les données depuis la base
    QSqlQueryModel *freshModel = gv->afficher();
    if (!freshModel) {
        QMessageBox::warning(this, "Erreur", "Impossible de charger les données des ventes !");
        return;
    }

    // Créer le proxy model pour le tri
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(freshModel);

    // Tri par prix TTC (colonne 5)
    proxyModel->sort(5, order);

    // Appliquer le modèle trié
    ui->tableView->setModel(proxyModel);

    // Ajuster les colonnes
    ui->tableView->resizeColumnsToContents();

    qDebug() << "✅ Ventes triées par prix TTC (" << orderText << ")";

    // Message utilisateur
    ui->statusbar->showMessage(QString("Ventes triées par prix TTC (%1)").arg(orderText), 3000);
}

void OpticalStore::on_Vrecherche_clicked()
{
    qDebug() << "🔍 Recherche de ventes par date";

    QString dateRecherche = ui->VRecherchetxt_2->text().trimmed();

    // Contrôle de saisie : champ vide
    if (dateRecherche.isEmpty()) {
        QMessageBox::information(this, "Champ vide",
                                 "Veuillez saisir une date au format JJ/MM/AAAA");
        return;
    }

    // Validation du format de date
    if (!isValidDate(dateRecherche)) {
        QMessageBox::warning(this, "Format invalide",
                             "Format de date invalide !\n"
                             "Veuillez utiliser le format JJ/MM/AAAA\n");
        return;
    }

    // Rechercher les ventes par date
    QSqlQueryModel *modelResultat = gv->rechercherParDate(dateRecherche);

    if (modelResultat) {
        if (modelResultat->rowCount() > 0) {
            ui->tableView->setModel(modelResultat);
            qDebug() << "✅" << modelResultat->rowCount() << "vente(s) trouvée(s) pour le" << dateRecherche;
        } else {
            ui->tableView->setModel(modelResultat);
            QMessageBox::information(this, "Aucun résultat",
                                     "Aucune vente trouvée pour la date : " + dateRecherche);
        }
    } else {
        QMessageBox::critical(this, "Erreur", "Erreur lors de la recherche !");
    }
}

void OpticalStore::on_VRecherchetxt_2_textChanged(const QString &text)
{
    qDebug() << "=== on_VRecherchetxt_2_textChanged ===";
    qDebug() << "Texte:" << text;
    qDebug() << "Est vide:" << text.trimmed().isEmpty();

    // Si le champ de recherche est vide, réafficher toutes les ventes
    if (text.trimmed().isEmpty()) {
        ui->tableView->setModel(gv->afficher());
        qDebug() << "✅ Affichage de toutes les ventes";
    } else {
        qDebug() << "❌ Champ non vide, pas de changement";
    }
}

bool OpticalStore::isValidDate(const QString &dateStr)
{
    // Expression régulière pour le format JJ/MM/AAAA
    QRegularExpression regex("^(0[1-9]|[12][0-9]|3[01])/(0[1-9]|1[0-2])/(\\d{4})$");
    QRegularExpressionMatch match = regex.match(dateStr);

    if (!match.hasMatch()) {
        return false;
    }

    // Extraire les composants de la date
    int jour = match.captured(1).toInt();
    int mois = match.captured(2).toInt();
    int annee = match.captured(3).toInt();

    // Validation de la date réelle
    QDate date(annee, mois, jour);
    return date.isValid();
}

// ===================== MODIFICATION ET SUPPRESSION CONTENIR =====================

void OpticalStore::on_modifierContenir_clicked()
{
    qDebug() << "=== MODIFICATION QUANTITÉ CONTENIR ===";

    // Vérifier si une ligne est sélectionnée dans dtableView3
    QModelIndexList selectedIndexes = ui->dtableView3->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty()) {
        QMessageBox::warning(this, "Sélection manquante", "Veuillez sélectionner une ligne à modifier dans le tableau CONTENIR!");
        return;
    }

    // Vérifier si une nouvelle quantité est saisie
    if (ui->quantiteDetail->text().isEmpty()) {
        QMessageBox::warning(this, "Quantité manquante", "Veuillez saisir la nouvelle quantité dans le champ!");
        return;
    }

    int row = selectedIndexes.first().row();
    QAbstractItemModel* model = ui->dtableView3->model();

    // Récupérer les informations de la ligne sélectionnée
    int idVente = model->data(model->index(row, 0)).toInt();
    int idProduit = model->data(model->index(row, 2)).toInt();
    int ancienneQuantite = model->data(model->index(row, 5)).toInt();
    double prixUnitaire = model->data(model->index(row, 6)).toDouble();
    QString nomProduit = model->data(model->index(row, 3)).toString();

    // Récupérer la nouvelle quantité depuis le champ
    int nouvelleQuantite = ui->quantiteDetail->text().toInt();

    qDebug() << "📋 Ligne à modifier - Vente:" << idVente << "Produit:" << idProduit
             << "Ancienne quantité:" << ancienneQuantite << "Nouvelle quantité:" << nouvelleQuantite << "Prix:" << prixUnitaire;

    if (nouvelleQuantite <= 0) {
        QMessageBox::warning(this, "Quantité invalide", "La quantité doit être positive!");
        return;
    }

    if (nouvelleQuantite == ancienneQuantite) {
        QMessageBox::information(this, "Aucun changement", "La quantité n'a pas été modifiée.");
        return;
    }

    // Vérifier le stock disponible (si augmentation)
    if (nouvelleQuantite > ancienneQuantite) {
        int quantiteSupplementaire = nouvelleQuantite - ancienneQuantite;

        QSqlQuery stockQuery;
        stockQuery.prepare("SELECT QUANTITE_PRODUIT FROM PRODUIT WHERE ID_PRODUIT = :id_produit");
        stockQuery.bindValue(":id_produit", idProduit);

        if (stockQuery.exec() && stockQuery.next()) {
            int stockDisponible = stockQuery.value(0).toInt();
            if (quantiteSupplementaire > stockDisponible) {
                QMessageBox::warning(this, "Stock insuffisant",
                                     QString("Stock disponible: %1\nQuantité supplémentaire demandée: %2")
                                         .arg(stockDisponible)
                                         .arg(quantiteSupplementaire));
                return;
            }
        }
    }

    // Confirmation de modification
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirmation de modification",
                                  QString("Confirmer la modification :\n\n"
                                          "Vente: %1\n"
                                          "Produit: %2 (%3)\n"
                                          "Ancienne quantité: %4\n"
                                          "Nouvelle quantité: %5\n"
                                          "Nouveau total: %6 €")
                                      .arg(idVente)
                                      .arg(idProduit)
                                      .arg(nomProduit)
                                      .arg(ancienneQuantite)
                                      .arg(nouvelleQuantite)
                                      .arg(nouvelleQuantite * prixUnitaire, 0, 'f', 2),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

    // Modifier dans la base de données
    if (gv->modifierDetailVente(idVente, idProduit, nouvelleQuantite, prixUnitaire)) {
        QMessageBox::information(this, "Succès", "Quantité modifiée avec succès !");

        // Ajuster le stock du produit
        int differenceQuantite = nouvelleQuantite - ancienneQuantite;
        QSqlQuery updateStock;
        updateStock.prepare("UPDATE PRODUIT SET QUANTITE_PRODUIT = QUANTITE_PRODUIT - :difference WHERE ID_PRODUIT = :id_produit");
        updateStock.bindValue(":difference", differenceQuantite);
        updateStock.bindValue(":id_produit", idProduit);
        if (updateStock.exec()) {
            qDebug() << "✅ Stock ajusté pour le produit:" << idProduit << "Différence:" << differenceQuantite;
        } else {
            qDebug() << "❌ Erreur ajustement stock:" << updateStock.lastError().text();
        }

        // Désélectionner la ligne et désactiver les boutons
        ui->dtableView3->clearSelection();
        ui->modifierContenir->setEnabled(false);
        ui->supprimerContenir->setEnabled(false);

        // Rafraîchir les affichages
        afficherDetailsContenir();
        afficherDetailsProduits(); // Pour mettre à jour les stocks affichés

        // Réinitialiser les champs
        ui->quantiteDetail->clear();
        ui->totalDetail->clear();
        selectedVenteId = -1;
        selectedProduitId = -1;
        selectedProduitPrix = 0;

        // Mettre à jour l'indicateur
        mettreAJourIndicateurs();

        qDebug() << "✅ Quantité modifiée - Vente:" << idVente << "Produit:" << idProduit
                 << "Ancienne:" << ancienneQuantite << "Nouvelle:" << nouvelleQuantite;

    } else {
        QMessageBox::critical(this, "Erreur", "Échec de la modification !");
    }
}

void OpticalStore::on_supprimerContenir_clicked()
{
    qDebug() << "=== SUPPRESSION LIGNE CONTENIR ===";

    // Vérifier si une ligne est sélectionnée dans dtableView3
    QModelIndexList selectedIndexes = ui->dtableView3->selectionModel()->selectedRows();
    if (selectedIndexes.isEmpty()) {
        QMessageBox::warning(this, "Sélection manquante", "Veuillez sélectionner une ligne à supprimer dans le tableau CONTENIR!");
        return;
    }

    int row = selectedIndexes.first().row();
    QAbstractItemModel* model = ui->dtableView3->model();

    // Récupérer les informations de la ligne sélectionnée
    int idVente = model->data(model->index(row, 0)).toInt();
    int idProduit = model->data(model->index(row, 2)).toInt();
    int quantite = model->data(model->index(row, 5)).toInt();
    QString nomProduit = model->data(model->index(row, 3)).toString();

    qDebug() << "📋 Ligne à supprimer - Vente:" << idVente << "Produit:" << idProduit << "Quantité:" << quantite;

    // Confirmation de suppression
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirmation de suppression",
                                  QString("Êtes-vous sûr de vouloir supprimer cette association ?\n\n"
                                          "Vente: %1\n"
                                          "Produit: %2 (%3)\n"
                                          "Quantité: %4")
                                      .arg(idVente)
                                      .arg(idProduit)
                                      .arg(nomProduit)
                                      .arg(quantite),
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply != QMessageBox::Yes) {
        return;
    }

    // Supprimer de la base de données
    if (gv->supprimerDetailVente(idVente, idProduit)) {
        QMessageBox::information(this, "Succès", "Association supprimée avec succès !");

        // Restaurer le stock du produit
        QSqlQuery updateStock;
        updateStock.prepare("UPDATE PRODUIT SET QUANTITE_PRODUIT = QUANTITE_PRODUIT + :quantite WHERE ID_PRODUIT = :id_produit");
        updateStock.bindValue(":quantite", quantite);
        updateStock.bindValue(":id_produit", idProduit);
        if (updateStock.exec()) {
            qDebug() << "✅ Stock restauré pour le produit:" << idProduit;
        } else {
            qDebug() << "❌ Erreur restauration stock:" << updateStock.lastError().text();
        }

        // Désélectionner la ligne et désactiver les boutons
        ui->dtableView3->clearSelection();
        ui->modifierContenir->setEnabled(false);
        ui->supprimerContenir->setEnabled(false);

        // Rafraîchir les affichages
        afficherDetailsContenir();
        afficherDetailsProduits(); // Pour mettre à jour les stocks affichés

        // Réinitialiser les champs
        ui->quantiteDetail->clear();
        ui->totalDetail->clear();
        selectedVenteId = -1;
        selectedProduitId = -1;
        selectedProduitPrix = 0;

        // Mettre à jour l'indicateur
        mettreAJourIndicateurs();

        qDebug() << "✅ Association supprimée - Vente:" << idVente << "Produit:" << idProduit;

    } else {
        QMessageBox::critical(this, "Erreur", "Échec de la suppression !\nVérifiez que l'association existe.");
    }
}

// ===================== RECHERCHE CONTENIR =====================

void OpticalStore::on_rechercheContenir_clicked()
{
    qDebug() << "🔍 Recherche dans CONTENIR par date";

    QString dateRecherche = ui->rechercheContenirTxt->text().trimmed();

    // Si le champ est vide, afficher tout
    if (dateRecherche.isEmpty()) {
        afficherDetailsContenir();
        QMessageBox::information(this, "Recherche", "Affichage de tous les détails de vente.");
        return;
    }

    // Validation du format de date
    if (!isValidDate(dateRecherche)) {
        QMessageBox::warning(this, "Format invalide",
                             "Format de date invalide !\n"
                             "Veuillez utiliser le format JJ/MM/AAAA\n"
                             "Exemple: 25/12/2024");
        return;
    }

    // Rechercher les détails par date
    QSqlQueryModel *modelResultat = gv->rechercherDetailsParDate(dateRecherche);

    if (modelResultat) {
        if (modelResultat->rowCount() > 0) {
            ui->dtableView3->setModel(modelResultat);

            // Ajuster les largeurs de colonnes
            ui->dtableView3->setColumnWidth(0, 80);
            ui->dtableView3->setColumnWidth(1, 100);
            ui->dtableView3->setColumnWidth(2, 80);
            ui->dtableView3->setColumnWidth(3, 160);
            ui->dtableView3->setColumnWidth(4, 130);
            ui->dtableView3->setColumnWidth(5, 70);
            ui->dtableView3->setColumnWidth(6, 110);
            ui->dtableView3->setColumnWidth(7, 110);

            qDebug() << "✅" << modelResultat->rowCount() << "ligne(s) trouvée(s) pour le" << dateRecherche;

            // Mettre à jour l'indicateur
            mettreAJourIndicateurs();

        } else {
            ui->dtableView3->setModel(modelResultat);
            QMessageBox::information(this, "Aucun résultat",
                                     "Aucun détail de vente trouvé pour la date : " + dateRecherche);
        }
    } else {
        QMessageBox::critical(this, "Erreur", "Erreur lors de la recherche !");
    }
}

void OpticalStore::on_rechercheContenirTxt_textChanged(const QString &text)
{
    qDebug() << "=== on_rechercheContenirTxt_textChanged ===";
    qDebug() << "Texte:" << text;
    qDebug() << "Est vide:" << text.trimmed().isEmpty();

    // Si le champ de recherche est vide, réafficher tous les détails
    if (text.trimmed().isEmpty()) {
        afficherDetailsContenir();
        qDebug() << "✅ Affichage de tous les détails CONTENIR";
    } else {
        qDebug() << "📝 Saisie en cours, attente du bouton recherche";
    }
}

// ===================== EXPORTATION CONTENIR =====================

void OpticalStore::on_exporterContenir_clicked()
{
    qDebug() << "📤 Exportation des détails de vente CONTENIR en PDF";

    // Vérifier rapidement s'il y a des données
    QSqlQuery checkQuery("SELECT COUNT(*) FROM CONTENIR");
    if (!checkQuery.next() || checkQuery.value(0).toInt() == 0) {
        QMessageBox::information(this, "Exportation", "Aucun détail de vente à exporter dans la base de données.");
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Exporter les détails de vente en PDF",
        "Details_Ventes_" + QDateTime::currentDateTime().toString("yyyyMMdd_HHmm") + ".pdf",
        "Fichiers PDF (*.pdf)"
        );

    if (fileName.isEmpty()) {
        return;
    }

    // Assurer l'extension .pdf
    if (!fileName.endsWith(".pdf", Qt::CaseInsensitive)) {
        fileName += ".pdf";
    }

    // Exporter directement
    bool success = gv->exporterPDFDetails(fileName);

    if (success) {
        QMessageBox::information(this, "Exportation réussie",
                                 "✅ Les détails de vente ont été exportés en PDF avec succès !\n\n"
                                 "Fichier : " + fileName);

        // Optionnel : ouvrir le fichier après exportation
        QMessageBox::StandardButton openFile = QMessageBox::question(this, "Ouvrir le fichier",
                                                                     "Voulez-vous ouvrir le fichier PDF maintenant ?",
                                                                     QMessageBox::Yes | QMessageBox::No);
        if (openFile == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
        }
    } else {
        QMessageBox::critical(this, "Exportation échouée",
                              "❌ L'exportation a échoué. Vérifiez les données et les permissions.");
    }
}

// ===================== NAVIGATION =====================

void OpticalStore::on_Vdetail_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_9);
}

void OpticalStore::on_vdetailretour_clicked()
{
    ui->stackedWidget->setCurrentWidget(ui->page_5);

}


//eya
void OpticalStore::exporterProduitsEnPDF()
{
    QTableWidget *table = ui->tableWidget_produits;
    if (!table || table->rowCount() == 0) {
        QMessageBox::warning(this, "Export PDF", "Le tableau est vide !");
        return;
    }

    QString fichier = QFileDialog::getSaveFileName(this, "Exporter en PDF", "", "*.pdf");
    if (fichier.isEmpty()) return;
    if (!fichier.endsWith(".pdf")) fichier += ".pdf";

    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fichier);

    QTextDocument doc;
    QTextCursor cursor(&doc);

    // Titre
    cursor.insertHtml("<h2>Liste des produits</h2><br>");

    // Créer le tableau
    QTextTableFormat tableFormat;
    tableFormat.setBorder(1);
    tableFormat.setCellPadding(4);
    tableFormat.setAlignment(Qt::AlignCenter);

    QTextTable *textTable = cursor.insertTable(table->rowCount() + 1, table->columnCount(), tableFormat);

    // En-têtes
    for (int col = 0; col < table->columnCount(); ++col) {
        textTable->cellAt(0, col).firstCursorPosition().insertText(
            table->horizontalHeaderItem(col)->text()
            );
    }

    // Contenu
    for (int row = 0; row < table->rowCount(); ++row) {
        for (int col = 0; col < table->columnCount(); ++col) {
            QString txt = table->item(row, col) ? table->item(row, col)->text() : "";
            textTable->cellAt(row + 1, col).firstCursorPosition().insertText(txt);
        }
    }

    doc.print(&printer);
    QMessageBox::information(this, "Export PDF", "Exportation réussie : " + fichier);
}

void OpticalStore::on_comboBoxTri_currentIndexChanged(const QString &critere)
{
    int col = 0;
    if (critere == "Nom") col = 1;
    else if (critere == "Catégorie") col = 2;
    else if (critere == "Quantité") col = 4;
    else if (critere == "Prix") col = 5;

    ui->tableWidget_produits->sortItems(col, Qt::AscendingOrder);
}


void OpticalStore::on_lineEdit_recherche_textChanged(const QString &texte)
{
    gs.rechercherParNomOuId(ui->tableWidget_produits, texte);
}

void OpticalStore::rafraichirStockSecurite()
{
    // Remplir dynamiquement le tableau des produits en dessous du seuil
    stock.mettreAJourStockSecurite(ui->tableStockSecurite);
}
void OpticalStore::afficherStockProduits()
{
    stock.afficherProduits(ui->tableWidget_produits);
}
void OpticalStore::on_btn_ligne_vente_clicked() { ui->stackedWidget->setCurrentWidget(ui->page_lignes_vente); }

void OpticalStore::remplirTableStockSecurite() {
    QTableWidget *table = ui->tableStockSecurite;
    if (!table) {
        qDebug() << "Erreur: tableStockSecurite n'est pas initialisé.";
        return;
    }

    table->clear();
    table->setRowCount(0);
    table->setColumnCount(6);
    table->setHorizontalHeaderLabels({"ID_Produit", "Nom_Produit", "Catégorie", "Marque_Produit", "Quantité_dispo", "Prix_Unitaire"});

    QSqlQuery query("SELECT ID_PRODUIT, NOM_PRODUIT, CATEGORIE, MARQUE, QUANTITE, PRIX_UNITAIRE "
                    "FROM PRODUIT WHERE QUANTITE <= SEUIL_SECURITE ORDER BY QUANTITE ASC");

    if (!query.exec()) {
        qDebug() << "Erreur SQL stock sécurité :" << query.lastError().text();
        return;
    }

    int row = 0;
    while (query.next()) {
        table->insertRow(row);
        for (int col = 0; col < 6; ++col) {
            QTableWidgetItem *item = nullptr;
            if (col == 0 || col == 4 || col == 5) {  // Colonnes numériques: ID (0), Quantité (4), Prix (5)
                double value = query.value(col).toDouble();
                QString text = (col == 5) ? QString::number(value, 'f', 2) : QString::number(static_cast<int>(value));
                item = new NumericItem(text);
            } else {
                item = new QTableWidgetItem(query.value(col).toString());
            }
            item->setForeground(Qt::black);
            table->setItem(row, col, item);
        }
        row++;
    }

    if (row == 0) {
        qDebug() << "Aucun produit sous le seuil de sécurité.";
    }
}
void OpticalStore::afficherStatsCategorie()
{
    if (!ui->tableWidget_produits) return;

    // 1) Agréger quantités par catégorie depuis le tableau (normalisé)
    QMap<QString, int> stats;
    int rowCount = ui->tableWidget_produits->rowCount();
    const int colCategorie = 2;
    const int colQuantite  = 4;

    for (int r = 0; r < rowCount; ++r) {
        QTableWidgetItem *catItem = ui->tableWidget_produits->item(r, colCategorie);
        QTableWidgetItem *qteItem = ui->tableWidget_produits->item(r, colQuantite);
        if (!catItem || !qteItem) continue;

        QString categorie = catItem->text().trimmed();
        if (categorie.isEmpty()) categorie = QStringLiteral("Sans catégorie");

        bool ok = false;
        int qte = qteItem->text().trimmed().toInt(&ok);
        if (!ok) {
            QVariant v = qteItem->data(Qt::EditRole);
            if (v.isValid()) qte = v.toInt(&ok);
        }
        if (!ok) qte = 0;

        stats[categorie] += qte;
    }

    // Si rien à afficher, vider et afficher message
    double total = 0.0;
    for (auto v : stats) total += v;
    if (total <= 0.0) {
        if (QLayout *old = ui->groupBox_stats->layout()) {
            QLayoutItem *it;
            while ((it = old->takeAt(0)) != nullptr) {
                if (it->widget()) delete it->widget();
                delete it;
            }
            delete old;
        }
        QVBoxLayout *emptyLay = new QVBoxLayout(ui->groupBox_stats);
        QLabel *lbl = new QLabel("Aucune donnée à afficher", ui->groupBox_stats);
        lbl->setAlignment(Qt::AlignCenter);
        emptyLay->addWidget(lbl);
        ui->groupBox_stats->setLayout(emptyLay);
        return;
    }

    // 2) Nettoyer ancien layout
    if (QLayout *oldLayout = ui->groupBox_stats->layout()) {
        QLayoutItem* item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            if (item->widget()) delete item->widget();
            delete item;
        }
        delete oldLayout;
    }

    // 3) Palette de couleurs (modifiable)
    const QList<QColor> palette = {
        QColor("#4e79a7"), QColor("#f28e2b"), QColor("#e15759"),
        QColor("#76b7b2"), QColor("#59a14f"), QColor("#edc949"),
        QColor("#af7aa1"), QColor("#ff9da7")
    };

    // 4) Construire la série et forcer couleurs (pour correspondance légende)
    QPieSeries *series = new QPieSeries();
    int colorIndex = 0;
    // garder une liste de tuples (cat, value, color) pour la légende ordonnée
    struct LegendEntry { QString cat; int value; QColor color; };
    QList<LegendEntry> legendEntries;

    for (auto it = stats.constBegin(); it != stats.constEnd(); ++it) {
        int val = it.value();
        if (val <= 0) continue;
        QString cat = it.key();
        QPieSlice *slice = series->append(cat, val);
        QColor c = palette.at(colorIndex % palette.size());
        slice->setBrush(QBrush(c));
        slice->setPen(QPen(Qt::white, 0.5));
        // stocker nom original dans property
        slice->setProperty("catName", cat);
        legendEntries.append({cat, val, c});
        colorIndex++;
    }

    // 5) Configurer labels (pourcentage) visibles, noirs, à l'extérieur
    for (QPieSlice *slice : series->slices()) {
        double perc = (slice->value() / total) * 100.0;
        slice->setLabel(QString("%1%").arg(QString::number(perc, 'f', 1)));
        slice->setLabelVisible(true);
        slice->setLabelColor(Qt::black);
        slice->setLabelPosition(QPieSlice::LabelOutside);
    }

    // 6) Chart + ChartView
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Répartition des produits par catégorie");
    chart->legend()->hide();
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundBrush(Qt::white);
    chart->setContentsMargins(0,0,0,0);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 7) Construire légende en dessous (carré couleur + texte "Nom — N unités (X%)")
    QWidget *legendWidget = new QWidget(ui->groupBox_stats);
    QVBoxLayout *legendVBox = new QVBoxLayout(legendWidget);
    legendVBox->setContentsMargins(2,2,2,2);
    legendVBox->setSpacing(4);

    for (const LegendEntry &e : legendEntries) {
        double perc = (double)e.value / total * 100.0;
        QLabel *colorBox = new QLabel();
        colorBox->setFixedSize(14,14);
        colorBox->setStyleSheet(QString("background-color:%1; border:1px solid #333;")
                                    .arg(e.color.name()));

        QLabel *txt = new QLabel(QString("%1 — %2 unités (%3%)")
                                     .arg(e.cat)
                                     .arg(e.value)
                                     .arg(QString::number(perc, 'f', 1)));
        txt->setStyleSheet("color: black;");
        txt->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

        QHBoxLayout *h = new QHBoxLayout();
        h->setContentsMargins(0,0,0,0);
        h->addWidget(colorBox);
        h->addSpacing(6);
        h->addWidget(txt);
        h->addStretch();

        QWidget *row = new QWidget();
        row->setLayout(h);
        legendVBox->addWidget(row);
    }

    QScrollArea *scroll = new QScrollArea(ui->groupBox_stats);
    scroll->setWidgetResizable(true);
    scroll->setWidget(legendWidget);
    scroll->setFrameStyle(QFrame::NoFrame);
    scroll->setStyleSheet("background: transparent;");

    // 8) Layout final (chart occupe ~ 65%, légende ~ 35%)
    QVBoxLayout *mainLay = new QVBoxLayout(ui->groupBox_stats);
    mainLay->setContentsMargins(6,6,6,6);
    mainLay->setSpacing(6);
    mainLay->addWidget(chartView, /*stretch=*/65);
    mainLay->addWidget(scroll, /*stretch=*/35);
    ui->groupBox_stats->setLayout(mainLay);

    ui->groupBox_stats->update();
    ui->groupBox_stats->repaint();
}

void OpticalStore::remplirTableLignesVente() {
    QTableWidget *table = ui->tableLignesVente;

    // Ne pas faire clear(), juste vider les lignes
    table->setRowCount(0);

    QSqlQuery query;
    query.prepare("SELECT lv.ID_LIGNE, lv.ID_VENTE, lv.ID_PRODUIT, p.NOM_PRODUIT, "
                  "lv.QUANTITE, lv.PRIX_UNITAIRE, lv.REMISE_POURCENT, lv.PRIX_TOTAL_LIGNE "
                  "FROM LIGNE_VENTE lv "
                  "JOIN PRODUIT p ON lv.ID_PRODUIT = p.ID_PRODUIT "
                  "ORDER BY lv.ID_LIGNE DESC");

    if (!query.exec()) {
        qDebug() << "Erreur SQL lignes vente:" << query.lastError().text();
        return;
    }

    int row = 0;
    while (query.next()) {
        table->insertRow(row);
        for (int col = 0; col < 8; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(query.value(col).toString());

            // Optionnel : alignement centré pour être plus lisible
            item->setTextAlignment(Qt::AlignCenter);

            // Pas besoin de forcer couleur ou police → héritage du .ui
            table->setItem(row, col, item);
        }
        row++;
    }

    qDebug() << "Lignes de vente affichées:" << row;
}

void OpticalStore::clearLigneVenteForm()
{
    ui->lineEdit_id_ligne->clear();
    ui->comboBox_vente->setCurrentIndex(0); // Réinitialiser à "Sélectionnez une vente"
    ui->comboBox_produit_nom->setCurrentIndex(0); // Réinitialiser à "Sélectionnez un produit"
    ui->comboBox_id_produit->setCurrentIndex(0); // Réinitialiser à "Sélectionnez un produit"
    ui->lineEdit_quantite_ligne->clear();
    ui->lineEdit_prix_unitaire_ligne->clear();
    ui->lineEdit_remise_ligne->setText("0");
}
int OpticalStore::getSelectedLigneID() {
    int row = ui->tableLignesVente->currentRow();
    if (row >= 0 && ui->tableLignesVente->item(row, 0)) {
        return ui->tableLignesVente->item(row, 0)->text().toInt();
    }
    return -1;
}



void OpticalStore::on_btn_ajouter_ligne_clicked()
{
    // Récupérer les valeurs
    int id_vente = ui->comboBox_vente->currentText().toInt();

    // Utiliser le combobox des IDs pour récupérer l'ID du produit
    int id_produit = ui->comboBox_id_produit->currentData().toInt();
    QString nom_produit = ui->comboBox_produit_nom->currentText();

    // Récupérer les autres valeurs
    QString quantiteText = ui->lineEdit_quantite_ligne->text().trimmed();
    QString prixText = ui->lineEdit_prix_unitaire_ligne->text().trimmed();
    QString remiseText = ui->lineEdit_remise_ligne->text().trimmed();

    // ==================== VALIDATIONS DES CHAMPS ====================

    // 1. Vérification des champs vides
    if (quantiteText.isEmpty() || prixText.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Tous les champs doivent être remplis !");
        return;
    }

    // 2. Conversion et validation des types
    bool okQuantite, okPrix, okRemise;
    int quantite = quantiteText.toInt(&okQuantite);
    double prix_unitaire = prixText.toDouble(&okPrix);
    double remise = remiseText.isEmpty() ? 0.0 : remiseText.toDouble(&okRemise);

    if (!okQuantite || !okPrix) {
        QMessageBox::warning(this, "Erreur", "Valeurs invalides dans les champs !");
        return;
    }

    // 3. Validation de l'ID vente
    if (id_vente <= 0) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner une vente !");
        return;
    }

    // 4. Validation de l'ID produit
    if (id_produit <= 0) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner un produit !");
        return;
    }

    // 5. Validation de la quantité
    if (quantite <= 0) {
        QMessageBox::warning(this, "Erreur", "La quantité doit être positive !");
        return;
    }

    // 6. Validation du prix unitaire
    if (prix_unitaire <= 0) {
        QMessageBox::warning(this, "Erreur", "Le prix unitaire doit être positif !");
        return;
    }

    // 7. Validation de la remise
    if (remise < 0 || remise > 100) {
        QMessageBox::warning(this, "Erreur", "La remise doit être entre 0 et 100% !");
        return;
    }

    // ==================== VÉRIFICATION DU STOCK ====================

    // Vérifier le stock disponible
    QSqlQuery stockQuery;
    stockQuery.prepare("SELECT QUANTITE FROM PRODUIT WHERE ID_PRODUIT = :id_produit");
    stockQuery.bindValue(":id_produit", id_produit);

    if (!stockQuery.exec()) {
        QMessageBox::critical(this, "Erreur", "Impossible de vérifier le stock !");
        return;
    }

    if (stockQuery.next()) {
        int stock_disponible = stockQuery.value(0).toInt();
        if (quantite > stock_disponible) {
            QMessageBox::warning(this, "Stock insuffisant",
                                 QString("Stock disponible : %1\nQuantité demandée : %2")
                                     .arg(stock_disponible)
                                     .arg(quantite));
            return;
        }
    } else {
        QMessageBox::critical(this, "Erreur", "Produit non trouvé !");
        return;
    }

    // ==================== VÉRIFICATION DES DONNÉES ====================

    // Vérifier si le produit existe
    if (!stock.produitExiste(id_produit)) {
        QMessageBox::warning(this, "Erreur", "Le produit sélectionné n'existe plus !");
        return;
    }

    // Vérifier si la vente existe
    QSqlQuery checkVente;
    checkVente.prepare("SELECT COUNT(*) FROM VENTES WHERE ID_VENTE = :id");
    checkVente.bindValue(":id", id_vente);
    if (checkVente.exec() && checkVente.next() && checkVente.value(0).toInt() == 0) {
        QMessageBox::warning(this, "Erreur", "La vente sélectionnée n'existe pas !");
        return;
    }

    // ==================== RÉCUPÉRATION AUTOMATIQUE DES DONNÉES ====================

    // Si le prix n'est pas renseigné, le récupérer du produit via gestionstock
    if (prix_unitaire <= 0) {
        if (!stock.recupererPrixProduit(id_produit, prix_unitaire)) {
            QMessageBox::critical(this, "Erreur", "Impossible de récupérer le prix du produit !");
            return;
        }
        ui->lineEdit_prix_unitaire_ligne->setText(QString::number(prix_unitaire, 'f', 2));
    }

    // Récupérer la remise de la table VENTES via gestionstock
    double remise_vente = 0.0;
    if (stock.recupererRemiseVente(id_vente, remise_vente)) {
        if (remise_vente > 0) {
            remise = remise_vente;
            ui->lineEdit_remise_ligne->setText(QString::number(remise, 'f', 2));
        }
    }

    // ==================== CRÉATION ET AJOUT ====================

    // Créer l'objet LigneVente
    LigneVente lv(0, id_vente, id_produit, quantite, prix_unitaire, remise);

    // Calculer le total pour le message
    double total = lv.getPrixTotalLigne();

    // Message de confirmation
    QMessageBox::StandardButton confirm = QMessageBox::question(
        this, "Confirmation",
        QString("Confirmer l'ajout de la ligne de vente :\n\n"
                "Vente ID: %1\n"
                "Produit: %2 (ID: %3)\n"
                "Quantité: %4\n"
                "Prix unitaire: %5 €\n"
                "Remise: %6%\n"
                "Total ligne: %7 €")
            .arg(id_vente)
            .arg(nom_produit)
            .arg(id_produit)
            .arg(quantite)
            .arg(QString::number(prix_unitaire, 'f', 2))
            .arg(QString::number(remise, 'f', 2))
            .arg(QString::number(total, 'f', 2)),
        QMessageBox::Yes | QMessageBox::No);

    if (confirm != QMessageBox::Yes) {
        return;
    }

    // Ajouter la ligne
    if (lv.ajouter()) {
        QMessageBox::information(this, "Succès",
                                 QString("Ligne de vente ajoutée avec succès ✅\n\n"
                                         "Produit: %1\n"
                                         "Quantité: %2\n"
                                         "Total ligne: %3 €")
                                     .arg(nom_produit)
                                     .arg(quantite)
                                     .arg(QString::number(total, 'f', 2)));

        // Mettre à jour le stock dans la base
        QSqlQuery updateStock;
        updateStock.prepare("UPDATE PRODUIT SET QUANTITE = QUANTITE - :quantite WHERE ID_PRODUIT = :id_produit");
        updateStock.bindValue(":quantite", quantite);
        updateStock.bindValue(":id_produit", id_produit);
        updateStock.exec();

        // Mettre à jour les affichages
        remplirTableLignesVente();
        remplirTable();
        afficherStatsCategorie();
        clearLigneVenteForm();

        qDebug() << "Ajout réussi - ID Vente:" << id_vente
                 << "ID Produit:" << id_produit
                 << "Quantité:" << quantite
                 << "Total:" << total;
    } else {
        QMessageBox::critical(this, "Erreur",
                              "Échec de l'ajout !\n"
                              "Vérifiez :\n"
                              "- La connexion à la base de données\n"
                              "- L'intégrité des données");
    }
}
void OpticalStore::on_btn_modifier_ligne_clicked()
{
    // Récupérer l'ID depuis le champ
    QString idText = ui->lineEdit_id_ligne->text().trimmed();
    if (idText.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner une ligne à modifier !");
        return;
    }

    int id = idText.toInt();
    if (id <= 0) {
        QMessageBox::warning(this, "Erreur", "ID de ligne invalide !");
        return;
    }

    // Récupérer les nouvelles valeurs
    int id_vente = ui->comboBox_vente->currentText().toInt();

    // Utiliser le combobox des IDs pour récupérer l'ID du produit
    int id_produit = ui->comboBox_id_produit->currentData().toInt();
    QString nom_produit = ui->comboBox_produit_nom->currentText();

    QString quantiteText = ui->lineEdit_quantite_ligne->text().trimmed();
    QString prixText = ui->lineEdit_prix_unitaire_ligne->text().trimmed();
    QString remiseText = ui->lineEdit_remise_ligne->text().trimmed();

    // Vérification des champs vides
    if (quantiteText.isEmpty() || prixText.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Tous les champs doivent être remplis !");
        return;
    }

    bool ok1, ok2, ok3;
    int quantite = quantiteText.toInt(&ok1);
    double prix_unitaire = prixText.toDouble(&ok2);
    double remise = remiseText.isEmpty() ? 0.0 : remiseText.toDouble(&ok3);

    if (!ok1 || !ok2) {
        QMessageBox::warning(this, "Erreur", "Valeurs invalides dans les champs !");
        return;
    }

    if (id_vente <= 0) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner une vente !");
        return;
    }

    if (id_produit <= 0) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner un produit !");
        return;
    }

    if (quantite <= 0) {
        QMessageBox::warning(this, "Erreur", "La quantité doit être positive !");
        return;
    }

    if (prix_unitaire <= 0) {
        QMessageBox::warning(this, "Erreur", "Le prix unitaire doit être positif !");
        return;
    }

    if (remise < 0 || remise > 100) {
        QMessageBox::warning(this, "Erreur", "La remise doit être entre 0 et 100% !");
        return;
    }

    // Vérifier si la ligne existe
    QSqlQuery checkLine;
    checkLine.prepare("SELECT COUNT(*) FROM LIGNE_VENTE WHERE ID_LIGNE = :id");
    checkLine.bindValue(":id", id);
    if (checkLine.exec() && checkLine.next() && checkLine.value(0).toInt() == 0) {
        QMessageBox::warning(this, "Erreur", "La ligne à modifier n'existe plus !");
        clearLigneVenteForm();
        return;
    }

    // Vérifier si le produit existe
    if (!stock.produitExiste(id_produit)) {
        QMessageBox::warning(this, "Erreur", "Le produit sélectionné n'existe plus !");
        return;
    }

    // Vérifier si la vente existe
    QSqlQuery checkVente;
    checkVente.prepare("SELECT COUNT(*) FROM VENTES WHERE ID_VENTE = :id");
    checkVente.bindValue(":id", id_vente);
    if (checkVente.exec() && checkVente.next() && checkVente.value(0).toInt() == 0) {
        QMessageBox::warning(this, "Erreur", "La vente sélectionnée n'existe pas !");
        return;
    }

    // Vérifier si cette nouvelle association existe déjà (pour une autre ligne)
    QSqlQuery checkDuplicate;
    checkDuplicate.prepare("SELECT COUNT(*) FROM LIGNE_VENTE WHERE ID_VENTE = :id_vente AND ID_PRODUIT = :id_produit AND ID_LIGNE != :id");
    checkDuplicate.bindValue(":id_vente", id_vente);
    checkDuplicate.bindValue(":id_produit", id_produit);
    checkDuplicate.bindValue(":id", id);
    if (checkDuplicate.exec() && checkDuplicate.next() && checkDuplicate.value(0).toInt() > 0) {
        QMessageBox::warning(this, "Erreur",
                             "Cette association vente-produit existe déjà pour une autre ligne !");
        return;
    }

    // Récupérer les anciennes valeurs pour le message de confirmation
    QSqlQuery oldValues;
    oldValues.prepare("SELECT ID_VENTE, ID_PRODUIT, QUANTITE, PRIX_UNITAIRE, REMISE_POURCENT FROM LIGNE_VENTE WHERE ID_LIGNE = :id");
    oldValues.bindValue(":id", id);

    int old_id_vente = -1;
    int old_id_produit = -1;
    int old_quantite = -1;
    double old_prix = 0.0;
    double old_remise = 0.0;

    if (oldValues.exec() && oldValues.next()) {
        old_id_vente = oldValues.value(0).toInt();
        old_id_produit = oldValues.value(1).toInt();
        old_quantite = oldValues.value(2).toInt();
        old_prix = oldValues.value(3).toDouble();
        old_remise = oldValues.value(4).toDouble();
    }

    qDebug() << "Modification ligne:" << id
             << "\nAncien - Vente:" << old_id_vente << "Produit:" << old_id_produit << "Qte:" << old_quantite
             << "Prix:" << old_prix << "Remise:" << old_remise
             << "\nNouveau - Vente:" << id_vente << "Produit:" << id_produit << "(" << nom_produit << ")"
             << "Qte:" << quantite << "Prix:" << prix_unitaire << "Remise:" << remise;

    // Créer l'objet LigneVente
    LigneVente lv(id, id_vente, id_produit, quantite, prix_unitaire, remise);

    // Calculer le nouveau total
    double nouveau_total = lv.getPrixTotalLigne();

    // Message de confirmation détaillé
    QString message = "Confirmer la modification :\n\n";

    if (old_id_vente != id_vente) {
        message += QString("• Vente: %1 → %2\n").arg(old_id_vente).arg(id_vente);
    }

    if (old_id_produit != id_produit) {
        message += QString("• Produit: ID %1 → ID %2 (%3)\n").arg(old_id_produit).arg(id_produit).arg(nom_produit);
    }

    if (old_quantite != quantite) {
        message += QString("• Quantité: %1 → %2\n").arg(old_quantite).arg(quantite);
    }

    if (qAbs(old_prix - prix_unitaire) > 0.01) {
        message += QString("• Prix unitaire: %1 € → %2 €\n")
                       .arg(QString::number(old_prix, 'f', 2))
                       .arg(QString::number(prix_unitaire, 'f', 2));
    }

    if (qAbs(old_remise - remise) > 0.01) {
        message += QString("• Remise: %1% → %2%\n")
                       .arg(QString::number(old_remise, 'f', 2))
                       .arg(QString::number(remise, 'f', 2));
    }

    message += QString("\nNouveau total ligne: %1 €").arg(QString::number(nouveau_total, 'f', 2));

    QMessageBox::StandardButton confirm = QMessageBox::question(
        this, "Confirmation de modification", message, QMessageBox::Yes | QMessageBox::No);

    if (confirm != QMessageBox::Yes) {
        return;
    }

    // Effectuer la modification
    if (lv.modifier(id)) {
        QMessageBox::information(this, "Succès",
                                 QString("Ligne de vente modifiée avec succès ✅\n\n"
                                         "Nouveau total: %1 €")
                                     .arg(QString::number(nouveau_total, 'f', 2)));

        // Mettre à jour les affichages
        remplirTableLignesVente();
        remplirTable();
        afficherStatsCategorie();
        clearLigneVenteForm();

        qDebug() << "Modification réussie - ID Ligne:" << id
                 << "Produit:" << nom_produit << "ID Produit:" << id_produit;
    } else {
        QMessageBox::critical(this, "Erreur",
                              "Échec de la modification !\n"
                              "Vérifiez :\n"
                              "- Le stock disponible\n"
                              "- La connexion à la base de données\n"
                              "- L'intégrité des données");
    }
}





void OpticalStore::on_btn_supprimer_ligne_clicked() {
    int id = getSelectedLigneID();
    if (id < 0) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner une ligne à supprimer !");
        return;
    }
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Confirmation", "Voulez-vous vraiment supprimer cette ligne de vente ?",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        LigneVente lv;
        if (lv.supprimer(id)) {
            QMessageBox::information(this, "Succès", "Ligne de vente supprimée ✅");
            remplirTableLignesVente();
            remplirTable();
            afficherStatsCategorie();
            clearLigneVenteForm();
        } else {
            QMessageBox::critical(this, "Erreur", "Échec de la suppression !");
        }
    }
}

void OpticalStore::on_tableLignesVente_cellClicked(int row, int column)
{
    Q_UNUSED(column);

    if (row < 0) return;
    QTableWidget *table = ui->tableLignesVente;

    // Vérifier que la ligne a suffisamment de colonnes
    if (table->columnCount() < 8) {
        qDebug() << "Erreur: Tableau n'a pas assez de colonnes";
        return;
    }

    // Vérifier que tous les items existent
    for (int col = 0; col < 8; col++) {
        if (!table->item(row, col)) {
            qDebug() << "Erreur: Item manquant à la ligne" << row << "colonne" << col;
            return;
        }
    }

    // Récupérer les valeurs
    QString id_ligne = table->item(row, 0)->text();
    QString id_vente = table->item(row, 1)->text();
    QString id_produit_str = table->item(row, 2)->text();
    QString nom_produit = table->item(row, 3)->text();
    QString quantite = table->item(row, 4)->text();
    QString prix_unitaire = table->item(row, 5)->text();
    QString remise = table->item(row, 6)->text();
    QString total_ligne = table->item(row, 7)->text();

    // Mettre à jour les champs du formulaire
    ui->lineEdit_id_ligne->setText(id_ligne);

    // Sélectionner la vente dans la combobox
    int index_vente = ui->comboBox_vente->findText(id_vente);
    if (index_vente != -1) {
        ui->comboBox_vente->setCurrentIndex(index_vente);
    } else {
        qDebug() << "ID vente non trouvé:" << id_vente;
        // Optionnel: ajouter l'ID manquant à la combobox
        ui->comboBox_vente->addItem(id_vente);
        ui->comboBox_vente->setCurrentText(id_vente);
    }

    // Mettre à jour l'ID dans le combobox des IDs
    int id_produit = id_produit_str.toInt();

    // Synchroniser d'abord les combobox (sans vider l'ID sélectionné)
    synchroniserComboProduits();

    // Trouver l'index correspondant dans le combobox des IDs
    int idIndex = ui->comboBox_id_produit->findData(id_produit);
    if (idIndex >= 0) {
        ui->comboBox_id_produit->setCurrentIndex(idIndex);
    } else {
        // Si l'ID n'est pas trouvé, l'ajouter
        ui->comboBox_id_produit->addItem(QString("%1 - %2").arg(id_produit).arg(nom_produit), id_produit);
        ui->comboBox_id_produit->setCurrentIndex(ui->comboBox_id_produit->count() - 1);
    }

    // Trouver l'index correspondant dans le combobox des noms
    int nomIndex = ui->comboBox_produit_nom->findData(id_produit);
    if (nomIndex >= 0) {
        ui->comboBox_produit_nom->setCurrentIndex(nomIndex);
    } else {
        // Si le produit n'est pas dans la liste, l'ajouter
        ui->comboBox_produit_nom->addItem(nom_produit, id_produit);
        ui->comboBox_produit_nom->setCurrentIndex(ui->comboBox_produit_nom->count() - 1);
    }

    // Mettre à jour les autres champs
    ui->lineEdit_quantite_ligne->setText(quantite);
    ui->lineEdit_prix_unitaire_ligne->setText(prix_unitaire);
    ui->lineEdit_remise_ligne->setText(remise);

    // Récupérer et afficher le prix du produit
    double prix = 0.0;
    if (stock.recupererPrixProduit(id_produit, prix)) {
        ui->lineEdit_prix_unitaire_ligne->setText(QString::number(prix, 'f', 2));
    }

    // Mettre en surbrillance la ligne sélectionnée
    for (int col = 0; col < table->columnCount(); col++) {
        QTableWidgetItem *item = table->item(row, col);
        if (item) {
            item->setBackground(QColor(255, 248, 220)); // Jaune très clair
        }
    }

    // Réinitialiser les couleurs des autres lignes
    for (int r = 0; r < table->rowCount(); r++) {
        if (r != row) {
            for (int col = 0; col < table->columnCount(); col++) {
                QTableWidgetItem *item = table->item(r, col);
                if (item) {
                    item->setBackground(Qt::white);
                }
            }
        }
    }

    qDebug() << "Ligne sélectionnée:"
             << "\nID:" << id_ligne
             << "\nVente:" << id_vente
             << "\nProduit ID:" << id_produit << "Nom:" << nom_produit
             << "\nQuantité:" << quantite
             << "\nPrix:" << prix_unitaire
             << "\nRemise:" << remise
             << "\nTotal:" << total_ligne;

    // Activer les boutons appropriés
    ui->btn_modifier_ligne->setEnabled(true);
    ui->btn_supprimer_ligne->setEnabled(true);
    ui->btn_ajouter_ligne->setEnabled(false); // Désactiver ajout pendant modification
}
void OpticalStore::on_produitSelectionne(int index) {
    qDebug() << "Produit sélectionné, index:" << index;

    if (index < 0) {
        ui->lineEdit_prix_unitaire_ligne->clear();
        return;
    }

    // Récupérer l'ID du produit sélectionné
    QVariant data = ui->comboBox_produit_nom->itemData(index);
    if (!data.isValid()) {
        ui->lineEdit_prix_unitaire_ligne->clear();
        return;
    }

    int id_produit = data.toInt();
    qDebug() << "ID Produit sélectionné:" << id_produit;

    if (id_produit > 0) {
        // Récupérer le prix du produit via gestionstock
        double prix = 0.0;
        if (stock.recupererPrixProduit(id_produit, prix)) {
            ui->lineEdit_prix_unitaire_ligne->setText(QString::number(prix, 'f', 2));
            qDebug() << "Prix récupéré:" << prix;
        } else {
            ui->lineEdit_prix_unitaire_ligne->clear();
            qDebug() << "Impossible de récupérer le prix pour le produit ID:" << id_produit;
        }
    } else {
        ui->lineEdit_prix_unitaire_ligne->clear();
    }
}

void OpticalStore::on_venteSelectionnee(int index) {
    qDebug() << "Vente sélectionnée, index:" << index;

    if (index < 0) {
        ui->lineEdit_remise_ligne->setText("0");
        return;
    }

    QString id_vente_text = ui->comboBox_vente->currentText();
    if (id_vente_text.isEmpty()) {
        ui->lineEdit_remise_ligne->setText("0");
        return;
    }

    int id_vente = id_vente_text.toInt();
    qDebug() << "ID Vente sélectionnée:" << id_vente;

    double remise = 0.0;
    if (stock.recupererRemiseVente(id_vente, remise)) {
        qDebug() << "Remise récupérée depuis table VENTES:" << remise;
        ui->lineEdit_remise_ligne->setText(QString::number(remise, 'f', 2));
    } else {
        // Si pas de remise dans la table VENTES, mettre 0
        ui->lineEdit_remise_ligne->setText("0");
        qDebug() << "Aucune remise trouvée pour vente ID:" << id_vente;
    }
}
void OpticalStore::on_btnRetour_2_clicked() { ui->stackedWidget->setCurrentWidget(ui->page_7); }

void OpticalStore::synchroniserComboProduits()
{
    // Sauvegarder la sélection actuelle (si nécessaire)
    int currentId = -1;
    int currentIndexId = ui->comboBox_id_produit->currentIndex();
    if (currentIndexId >= 0) {
        QVariant data = ui->comboBox_id_produit->itemData(currentIndexId);
        if (data.isValid()) {
            currentId = data.toInt();
        }
    }

    // Synchroniser le contenu complet
    ui->comboBox_id_produit->clear();

    // Ajouter l'option par défaut
    ui->comboBox_id_produit->addItem("Sélectionnez un produit", -1);

    // Ajouter tous les produits disponibles
    for (int i = 0; i < ui->comboBox_produit_nom->count(); ++i) {
        QVariant data = ui->comboBox_produit_nom->itemData(i);
        QString nom = ui->comboBox_produit_nom->itemText(i);
        if (data.isValid() && !nom.isEmpty() && data.toInt() != -1) {
            int id = data.toInt();
            ui->comboBox_id_produit->addItem(QString("%1 - %2").arg(id).arg(nom), id);
        }
    }

    // Restaurer la sélection si elle existe
    if (currentId != -1) {
        int indexToSelect = ui->comboBox_id_produit->findData(currentId);
        if (indexToSelect >= 0) {
            ui->comboBox_id_produit->setCurrentIndex(indexToSelect);
        } else {
            ui->comboBox_id_produit->setCurrentIndex(0); // Par défaut
        }
    } else {
        ui->comboBox_id_produit->setCurrentIndex(0); // Par défaut
    }

    // S'assurer que le comboBox_produit_nom est aussi à l'index 0
    ui->comboBox_produit_nom->setCurrentIndex(0);

    qDebug() << "Produits synchronisés :" << ui->comboBox_id_produit->count() << "éléments";
}
void OpticalStore::on_comboBox_produit_nom_currentIndexChanged(int index)
{
    // Empêcher les appels récursifs
    static bool inProgress = false;
    if (inProgress) return;
    inProgress = true;

    if (index >= 0) {
        QVariant data = ui->comboBox_produit_nom->itemData(index);
        if (data.isValid()) {
            int id_produit = data.toInt();

            // Mettre à jour le combobox des IDs
            int idIndex = ui->comboBox_id_produit->findData(id_produit);
            if (idIndex >= 0 && idIndex != ui->comboBox_id_produit->currentIndex()) {
                ui->comboBox_id_produit->setCurrentIndex(idIndex);
            }

            // Mettre à jour le prix
            double prix = 0.0;
            if (stock.recupererPrixProduit(id_produit, prix)) {
                ui->lineEdit_prix_unitaire_ligne->setText(QString::number(prix, 'f', 2));
            }
        }
    }

    inProgress = false;
}

void OpticalStore::on_comboBox_id_produit_currentIndexChanged(int index)
{
    // Empêcher les appels récursifs
    static bool inProgress = false;
    if (inProgress) return;
    inProgress = true;

    if (index >= 0) {
        QVariant data = ui->comboBox_id_produit->itemData(index);
        if (data.isValid()) {
            int id_produit = data.toInt();

            // Mettre à jour le combobox des noms
            int nomIndex = ui->comboBox_produit_nom->findData(id_produit);
            if (nomIndex >= 0 && nomIndex != ui->comboBox_produit_nom->currentIndex()) {
                ui->comboBox_produit_nom->setCurrentIndex(nomIndex);
            }

            // Mettre à jour le prix
            double prix = 0.0;
            if (stock.recupererPrixProduit(id_produit, prix)) {
                ui->lineEdit_prix_unitaire_ligne->setText(QString::number(prix, 'f', 2));
            }
        }
    }

    inProgress = false;
}
void OpticalStore::on_comboBox_vente_currentIndexChanged(int index)
{
    if (index < 0) return;

    QVariant data = ui->comboBox_vente->itemData(index);
    if (!data.isValid() || data.toInt() == -1) {
        // Option par défaut sélectionnée
        ui->lineEdit_remise_ligne->clear();
        return;
    }

    int id_vente = data.toInt();
    double remise = 0.0;

    if (stock.recupererRemiseVente(id_vente, remise)) {
        ui->lineEdit_remise_ligne->setText(QString::number(remise, 'f', 2));
    } else {
        ui->lineEdit_remise_ligne->setText("0");
    }
}
// Slots pour Arduino
void OpticalStore::onArduinoConnected(bool connected) {
    if (connected) {
        ui->statusbar->showMessage("✅ Arduino connecté", 3000);
    } else {
        ui->statusbar->showMessage("❌ Arduino non connecté", 3000);
    }
}

void OpticalStore::onArduinoError(const QString &error) {
    qDebug() << "Erreur Arduino:" << error;
    ui->statusbar->showMessage("Arduino: " + error, 3000);
}

// Fonction utilitaire pour afficher un client
void OpticalStore::afficherClientSurArduino(int idClient) {
    if (!arduino->isConnected()) {
        qDebug() << "Arduino non connecté, affichage ignoré";
        return;
    }

    // Récupérer infos client depuis BDD
    QSqlQuery query;
    query.prepare("SELECT NOM, PRENOM, FIDELITE FROM CLIENTS WHERE ID_CLIENT = :id");
    query.bindValue(":id", idClient);

    if (query.exec() && query.next()) {
        QString nom = query.value(0).toString();
        QString prenom = query.value(1).toString();
        int points = query.value(2).toInt();

        // Calculer remise basée sur les points
        int remise = 0;
        if (points >= 1000) remise = 50;
        else if (points >= 500) remise = 30;
        else if (points >= 200) remise = 15;
        else if (points >= 100) remise = 10;
        else if (points >= 50) remise = 5;

        // Envoyer à Arduino
        arduino->displayClient(idClient, nom, prenom, points, remise);

        qDebug() << QString("Client %1 %2 affiché (Points: %3, Remise: %4%)")
                        .arg(prenom).arg(nom).arg(points).arg(remise);
    }
}
void OpticalStore::afficherClientEtPoints(int idClient, int idVente)
{
    if (!arduino->isConnected()) {
        qDebug() << "Arduino non connecté, affichage ignoré";
        return;
    }

    qDebug() << "=== CALCUL REMISE AUTOMATIQUE ===";
    qDebug() << "ID Client:" << idClient << "ID Vente:" << idVente;

    // 1. Récupérer les points fidélité
    QSqlQuery clientQuery;
    clientQuery.prepare("SELECT NOM, PRENOM, FIDELITE FROM CLIENTS WHERE ID_CLIENT = :id");
    clientQuery.bindValue(":id", idClient);

    QString nom = "", prenom = "";
    int pointsFidelite = 0;

    if (clientQuery.exec() && clientQuery.next()) {
        nom = clientQuery.value(0).toString();
        prenom = clientQuery.value(1).toString();
        pointsFidelite = clientQuery.value(2).toInt();

        qDebug() << "✅ Client:" << prenom << nom << "Points:" << pointsFidelite;
    } else {
        qDebug() << "❌ Client non trouvé";
        return;
    }

    // 2. CALCULER LA REMISE AUTOMATIQUEMENT (pas de lecture depuis VENTES)
    double remiseTotale = 0.0;

    // Tableau de conversion points → remise
    if (pointsFidelite >= 1000) {
        remiseTotale = 5.0;
        qDebug() << "🎖️  Remise: 5% (Platine)";
    } else if (pointsFidelite >= 500) {
        remiseTotale = 3.0;
        qDebug() << "🥇 Remise: 3% (Or)";
    } else if (pointsFidelite >= 200) {
        remiseTotale = 2.0;
        qDebug() << "🥈 Remise: 2% (Argent)";
    } else if (pointsFidelite >= 100) {
        remiseTotale = 1.0;
        qDebug() << "🥉 Remise: 1% (Bronze)";
    } else if (pointsFidelite >= 50) {
        remiseTotale = 0.5;
        qDebug() << "🌟 Remise: 0.5% (Débutant)";
    } else {
        remiseTotale = 0.0;
        qDebug() << "ℹ️  Pas de remise (< 50 pts)";
    }

    qDebug() << "🎯 Remise calculée:" << remiseTotale << "%";

    // 3. Mettre à jour le champ remise (lecture seule)
    ui->Vremisetxt_2->setText(QString::number(remiseTotale, 'f', 2));

    // 4. Recalculer le prix TTC
    calculerPrixTTC();

    // 5. Envoyer à Arduino
    int remisePourArduino = static_cast<int>(remiseTotale);
    arduino->displaySale(idVente, idClient, nom, prenom, pointsFidelite, remisePourArduino);

    qDebug() << "📤 Envoyé à Arduino:" << pointsFidelite << "pts," << remiseTotale << "%";
}
void OpticalStore::calculerRemiseAuto(const QString &idClientText)
{
    // Si champ vide, remise = 0
    if (idClientText.trimmed().isEmpty()) {
        ui->Vremisetxt_2->setText("0");
        calculerPrixTTC();
        return;
    }

    // Vérifier que c'est un nombre valide
    bool ok;
    int idClient = idClientText.toInt(&ok);

    if (!ok || idClient <= 0) {
        ui->Vremisetxt_2->setText("0");
        calculerPrixTTC();
        return;
    }

    // Variables pour stocker les infos client
    QString nom = "";
    QString prenom = "";
    int points = 0;
    bool clientTrouve = false;

    // Récupérer les infos du client
    QSqlQuery query;
    query.prepare("SELECT NOM, PRENOM, FIDELITE FROM CLIENTS WHERE ID_CLIENT = :id");
    query.bindValue(":id", idClient);

    if (query.exec() && query.next()) {
        nom = query.value(0).toString();
        prenom = query.value(1).toString();
        points = query.value(2).toInt();
        clientTrouve = true;
    }

    if (!clientTrouve) {
        // Client non trouvé
        ui->Vremisetxt_2->setText("0");
        calculerPrixTTC();
        return;
    }

    // Calculer remise simple
    double remise = 0.0;

    if (points >= 1000) {
        remise = 50;
    } else if (points >= 500) {
        remise = 30;
    } else if (points >= 200) {
        remise = 20;
    } else if (points >= 100) {
        remise = 10;
    } else if (points >= 50) {
        remise = 5;
    }
    // Sinon remise reste à 0

    // Mettre à jour le champ
    ui->Vremisetxt_2->setText(QString::number(remise, 'f', 2));

    // Recalculer le prix TTC
    calculerPrixTTC();

    // ENVOYER À ARDUINO
    if (arduino && arduino->isConnected()) {
        int idVente = ui->Vidventetxt_2->text().toInt();
        if (idVente <= 0) idVente = 999; // Valeur par défaut si pas de vente

        int remisePourArduino = static_cast<int>(remise);
        arduino->displaySale(idVente, idClient, nom, prenom, points, remisePourArduino);

        qDebug() << "📤 Envoyé à Arduino: " << prenom << nom
                 << "| Points:" << points << "| Remise:" << remise << "%";
    }
}

// ==================== FONCTIONS CAPTEUR SANS BOUTON ====================

void OpticalStore::setupSensorDetection()
{
    if (arduino->isConnected()) {
        arduino->startSensorMonitoring();
        qDebug() << "🔍 Surveillance du capteur démarrée automatiquement";
        ui->statusbar->showMessage("✅ Capteur actif - Écrivez un ID client", 3000);

        // Activer le TextBox
        ui->txtIdClientCapteur->setEnabled(true);
        ui->txtIdClientCapteur->setFocus(); // Donner le focus au TextBox
    } else {
        ui->txtIdClientCapteur->setEnabled(false);
        ui->statusbar->showMessage("❌ Arduino non connecté", 3000);
    }
}


void OpticalStore::onSensorReady()
{
    ui->statusbar->showMessage("✅ Capteur prêt - Écrivez un ID client", 3000);
    ui->txtIdClientCapteur->setEnabled(true);

    // Afficher sur Arduino
    if (arduino->isConnected()) {
        arduino->displayMessage("Pret", "Ecrivez ID client");
    }
}

void OpticalStore::onObjectDetected()
{
    if (!canDetect) {
        qDebug() << "⏳ Détection ignorée (trop rapide)";
        return;
    }

    qDebug() << "🎯 OBJET DÉTECTÉ ! Vérification ID client...";

    // Empêcher les détections multiples pendant 2 secondes
    canDetect = false;
    detectionTimer->start();

    // Vérifier si un ID client est écrit dans le TextBox
    if (currentClientIdForDetection == -1) {
        qDebug() << "❌ Aucun ID client valide écrit";
        ui->statusbar->showMessage("Écrivez d'abord un ID client valide", 2000);

        // Afficher un message sur Arduino
        if (arduino->isConnected()) {
            arduino->displayMessage("Erreur", "ID manquant");
        }

        // Jouer un son d'erreur
        QApplication::beep();
        return;
    }

    // Vérifier si le client existe toujours
    if (!validateClientForPoints(currentClientIdForDetection)) {
        qDebug() << "❌ Client non trouvé:" << currentClientIdForDetection;
        ui->statusbar->showMessage("Client introuvable - Vérifiez l'ID", 2000);

        if (arduino->isConnected()) {
            arduino->displayMessage("Erreur", "Client inconnu");
        }
        currentClientIdForDetection = -1; // Réinitialiser
        return;
    }

    // Afficher un message sur Arduino
    if (arduino->isConnected()) {
        arduino->displayMessage("Detection", "Ajout points...");
    }

    // AJOUTER LES POINTS AUTOMATIQUEMENT
    addFidelityPoints(currentClientIdForDetection, 10);
}

void OpticalStore::on_txtIdClientCapteur_textChanged(const QString &text)
{
    QString trimmedText = text.trimmed();

    if (trimmedText.isEmpty()) {
        // TextBox vide - réinitialiser
        currentClientIdForDetection = -1;
        ui->statusbar->showMessage("Écrivez un ID client", 2000);
        return;
    }

    bool ok;
    int clientId = trimmedText.toInt(&ok);

    if (!ok || clientId <= 0) {
        // ID invalide
        currentClientIdForDetection = -1;
        ui->statusbar->showMessage("ID invalide - Doit être un nombre > 0", 2000);

        // Afficher sur Arduino
        if (arduino->isConnected()) {
            arduino->displayMessage("Erreur", "ID invalide");
        }
        return;
    }

    // Vérifier si le client existe
    if (!validateClientForPoints(clientId)) {
        currentClientIdForDetection = -1;
        ui->statusbar->showMessage("Client non trouvé - Vérifiez l'ID", 2000);

        // Afficher sur Arduino
        if (arduino->isConnected()) {
            arduino->displayMessage("Erreur", "Client inconnu");
        }
        return;
    }

    // ID client valide - le stocker
    currentClientIdForDetection = clientId;

    // Récupérer les infos du client pour affichage
    QSqlQuery query;
    query.prepare("SELECT NOM, PRENOM, FIDELITE FROM CLIENTS WHERE ID_CLIENT = :id");
    query.bindValue(":id", clientId);

    if (query.exec() && query.next()) {
        QString nom = query.value(0).toString();
        QString prenom = query.value(1).toString();
        int points = query.value(2).toInt();

        QString statusMessage = QString("✅ Client: %1 %2 (%3 points) - Prêt pour détection")
                                    .arg(prenom)
                                    .arg(nom)
                                    .arg(points);

        ui->statusbar->showMessage(statusMessage, 3000);

        // Afficher sur Arduino
        if (arduino->isConnected()) {
            // Calculer la remise actuelle
            int remise = 0;
            if (points >= 1000) remise = 5;
            else if (points >= 500) remise = 3;
            else if (points >= 200) remise = 2;
            else if (points >= 100) remise = 1;

            arduino->displayClient(clientId, nom, prenom, points, remise);
        }

        qDebug() << "✅ ID client valide:" << clientId << "-" << prenom << nom;
    }
}

bool OpticalStore::validateClientForPoints(int clientId)
{
    if (clientId <= 0) return false;

    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM CLIENTS WHERE ID_CLIENT = :id");
    query.bindValue(":id", clientId);

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }

    return false;
}

void OpticalStore::addFidelityPoints(int clientId, int points)
{
    if (clientId <= 0 || points <= 0) return;

    qDebug() << "➕ Ajout de" << points << "points pour client ID:" << clientId;

    // Récupérer les informations du client
    QSqlQuery query;
    query.prepare("SELECT NOM, PRENOM, FIDELITE FROM CLIENTS WHERE ID_CLIENT = :id");
    query.bindValue(":id", clientId);

    if (!query.exec() || !query.next()) {
        qDebug() << "❌ Client non trouvé lors de l'ajout";
        ui->statusbar->showMessage("❌ Client non trouvé", 2000);
        return;
    }

    QString nom = query.value(0).toString();
    QString prenom = query.value(1).toString();
    int pointsActuels = query.value(2).toInt();
    int nouveauxPoints = pointsActuels + points;

    // Mettre à jour la base de données
    QSqlQuery updateQuery;
    updateQuery.prepare("UPDATE CLIENTS SET FIDELITE = :points WHERE ID_CLIENT = :id");
    updateQuery.bindValue(":points", nouveauxPoints);
    updateQuery.bindValue(":id", clientId);

    if (updateQuery.exec()) {
        qDebug() << "✅ Points mis à jour:" << pointsActuels << "→" << nouveauxPoints;

        // Mettre à jour l'affichage dans le tableau
        updateClientFidelityDisplay(clientId, nouveauxPoints);

        // Afficher le message de confirmation
        QString message = QString("✅ +%1 points pour %2 %3 (Total: %4)")
                              .arg(points)
                              .arg(prenom)
                              .arg(nom)
                              .arg(nouveauxPoints);

        ui->statusbar->showMessage(message, 5000);

        // Mettre à jour l'affichage sur Arduino
        if (arduino->isConnected()) {
            // Recalculer la remise
            int remise = 0;
            if (nouveauxPoints >= 1000) remise = 5;
            else if (nouveauxPoints >= 500) remise = 3;
            else if (nouveauxPoints >= 200) remise = 2;
            else if (nouveauxPoints >= 100) remise = 1;

            arduino->displayClient(clientId, nom, prenom, nouveauxPoints, remise);
        }

        // Afficher une notification popup
        QMessageBox::information(this, "Points ajoutés",
                                 QString("✅ %1 points ajoutés automatiquement !\n\n"
                                         "Client: %2 %3\n"
                                         "Nouveau total: %4 points")
                                     .arg(points)
                                     .arg(prenom)
                                     .arg(nom)
                                     .arg(nouveauxPoints));

        // Jouer un son de succès
        QApplication::beep();

        // Effacer le TextBox après ajout (optionnel)
        ui->txtIdClientCapteur->clear();
        currentClientIdForDetection = -1;

    } else {
        qDebug() << "❌ Erreur mise à jour points:" << updateQuery.lastError().text();
        QMessageBox::warning(this, "Erreur",
                             "Impossible d'ajouter les points.\n"
                             "Erreur: " + updateQuery.lastError().text());
    }
}

void OpticalStore::updateClientFidelityDisplay(int clientId, int newPoints)
{
    // Mettre à jour le tableau des clients
    for (int row = 0; row < ui->tablelist->rowCount(); ++row) {
        if (ui->tablelist->item(row, 0)->text().toInt() == clientId) {
            ui->tablelist->item(row, 7)->setText(QString::number(newPoints));

            // Surligner la ligne mise à jour (vert clair)
            for (int col = 0; col < ui->tablelist->columnCount(); ++col) {
                QTableWidgetItem *item = ui->tablelist->item(row, col);
                if (item) {
                    item->setBackground(QColor(220, 255, 220));
                }
            }

            // Réinitialiser la couleur après 2 secondes
            QTimer::singleShot(2000, [this, row]() {
                for (int col = 0; col < ui->tablelist->columnCount(); ++col) {
                    QTableWidgetItem *item = ui->tablelist->item(row, col);
                    if (item) {
                        item->setBackground(Qt::white);
                    }
                }
            });

            break;
        }
    }

    // Rafraîchir l'affichage des clients
    afficherClients();
}
