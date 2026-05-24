#ifndef ARDUINO_H
#define ARDUINO_H

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QDebug>

class Arduino : public QObject
{
    Q_OBJECT

public:
    // Constructeur
    explicit Arduino(QObject *parent = nullptr);
    ~Arduino();

    // Connexion
    bool connectToArduino();
    bool isConnected() const;
    void disconnectArduino();

    // Envoi de données
    void displayClient(int clientId, const QString &nom, const QString &prenom,
                       int pointsFidelite, int remisePourcentage);
    void displayWelcome();
    void displayMessage(const QString &line1, const QString &line2);

    // Getters
    QString getPortName() const;
    QString getStatus() const;

    // Recherche automatique
    static QString findArduinoPort();
    void displaySale(int venteId, int clientId, const QString &nom, const QString &prenom,
                     int pointsFidelite, int remiseVente);

    // Gestion capteur
    void startSensorMonitoring();
    void stopSensorMonitoring();
    bool isMonitoring() const;

signals:
    void connected(bool success);
    void dataSent(const QString &data);
    void error(const QString &errorMessage);
    void statusChanged(const QString &status);
    void objectDetected();  // Nouveau signal pour détection d'objet
    void sensorReady();     // Signal lorsque le capteur est prêt

private:
    QSerialPort *serialPort;
    QString portName;
    bool connectedState;
    bool monitoring;
    QString buffer;

    // Méthodes privées
    bool openConnection(const QString &port);
    void writeData(const QString &data);
    QString formatClientData(int clientId, const QString &nom, const QString &prenom,
                             int points, int remise) const;
    void readData();
    void processSensorData(const QString &data);

};

#endif // ARDUINO_H
