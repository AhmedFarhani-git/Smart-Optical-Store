#ifndef GESTIONEMPLOYEE_H
#define GESTIONEMPLOYEE_H

#include <QString>
#include <QSqlQueryModel>
#include <QComboBox>  // Include QComboBox for ComboBox handling

class gestionemployee
{
public:
    gestionemployee();

    // Updated function signatures to handle 'embauche_emp' as an additional parameter
    bool ajouter(int id, QString nom, QString prenom, QString poste,
                 QString email, QString telephone, QString sexe,
                 QString situation, int enfants, QString mdp,
                 QString place, QString date, QString embauche_emp); // Added embauche_emp

    bool modifier(int id, QString nom, QString prenom, QString poste,
                  QString email, QString telephone, QString sexe,
                  QString situation, int enfants, QString mdp,
                  QString place, QString date, QString embauche_emp); // Added embauche_emp

    bool supprimer(int id);

    QSqlQueryModel* afficher();
    QSqlQueryModel* rechercher(const QString &text);
    QSqlQueryModel* trierAZ();
    QSqlQueryModel* trierAnciennete();

    // Add ComboBox for Poste (used in the UI to select poste)
    void setPosteComboBox(QComboBox *comboBox); // This is to pass the ComboBox to the class

private:
    QComboBox *posteComboBox;  // Declare QComboBox pointer for poste
};

#endif // GESTIONEMPLOYEE_H
