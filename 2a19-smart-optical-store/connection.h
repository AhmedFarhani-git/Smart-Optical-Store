#ifndef CONNECTION_H
#define CONNECTION_H
#include <QSqlDatabase>
#include <QDebug>
#include <QSqlError>

class connection
{
public:
    static connection& createInstance();
    bool createconnection();
private:
    QSqlDatabase db;
    connection();
    ~connection();
    connection(const connection&)= delete;
    connection& operator=(const connection&)= delete;
};

#endif // CONNECTION_H
