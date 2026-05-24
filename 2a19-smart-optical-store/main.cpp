#include <QApplication>
#include <QMessageBox>
#include "opticalstore.h"
#include "connection.h"
#include <QFile>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    connection& c =connection::createInstance();
    bool test = c.createconnection();

    if (test) {
        /*QMessageBox::information(
            nullptr,
            QObject::tr("Database is open"),
            QObject::tr("Connection successful.\nClick Cancel to exit."),
            QMessageBox::Cancel
            );*/
    }
    else {
        QMessageBox::critical(
            nullptr,
            QObject::tr("Database is not open"),
            QObject::tr("Connection failed.\nClick Cancel to exit."),
            QMessageBox::Cancel
            );

    }
    OpticalStore w;

    w.show();

    return a.exec();
}
