#include "connection.h"

connection::connection(){
    db = QSqlDatabase::addDatabase("QODBC");
}
connection::~connection(){
    if(db.isOpen()){
        db.close();
    }
}

connection& connection::createInstance(){
    static connection instance;
    return instance;
}

bool connection::createconnection()
{
    db.setDatabaseName("DS_LumaVision");
    db.setUserName("OpticalStore");
    db.setPassword("0000");


    if (db.open()) {
        qDebug()<<"connexion établie";
        return true;
    }
    qDebug()<<"échec de la connexion:"<<db.lastError().text();
    return false;
}
