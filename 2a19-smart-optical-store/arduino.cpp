#include "arduino.h"

Arduino::Arduino(QObject *parent)
    : QObject(parent),
    serialPort(nullptr),
    connectedState(false),
    monitoring(false)
{
    serialPort = new QSerialPort(this);

    // Configuration par défaut
    serialPort->setBaudRate(QSerialPort::Baud9600);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    // Connexion des signaux d'erreur
    connect(serialPort, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (error != QSerialPort::NoError) {
            emit this->error("Erreur série: " + serialPort->errorString());
            if (connectedState) {
                connectedState = false;
                emit connected(false);
            }
        }
    });

    // Connexion pour la lecture des données
    connect(serialPort, &QSerialPort::readyRead, this, &Arduino::readData);
}

Arduino::~Arduino()
{
    stopSensorMonitoring();
    if (serialPort && serialPort->isOpen()) {
        serialPort->close();
    }
}

bool Arduino::connectToArduino()
{
    // Si déjà connecté
    if (connectedState && serialPort->isOpen()) {
        return true;
    }

    // Chercher le port Arduino
    portName = findArduinoPort();
    if (portName.isEmpty()) {
        emit error("Aucun Arduino détecté. Vérifiez la connexion USB.");
        emit statusChanged("Non connecté");
        return false;
    }

    // Ouvrir la connexion en mode lecture/écriture
    if (openConnection(portName)) {
        connectedState = true;
        emit connected(true);
        emit statusChanged("Connecté sur " + portName);

        // Démarrer la surveillance du capteur
        startSensorMonitoring();

        qDebug() << "✅ Arduino connecté sur" << portName;
        return true;
    }

    return false;
}

bool Arduino::openConnection(const QString &port)
{
    serialPort->setPortName(port);

    if (serialPort->open(QIODevice::ReadWrite)) {
        // Petit délai pour que l'Arduino soit prêt
        QTimer::singleShot(1000, this, [this]() {
            if (serialPort->isOpen()) {
                qDebug() << "Connexion série établie en mode ReadWrite";
            }
        });
        return true;
    }

    emit error("Impossible d'ouvrir " + port + ": " + serialPort->errorString());
    return false;
}

bool Arduino::isConnected() const
{
    return connectedState && serialPort && serialPort->isOpen();
}

void Arduino::disconnectArduino()
{
    stopSensorMonitoring();

    if (serialPort->isOpen()) {
        serialPort->close();
    }
    connectedState = false;
    emit connected(false);
    emit statusChanged("Déconnecté");

    qDebug() << "Arduino déconnecté";
}

void Arduino::displayClient(int clientId, const QString &nom, const QString &prenom,
                            int pointsFidelite, int remisePourcentage)
{
    if (!isConnected()) {
        emit error("Arduino non connecté");
        return;
    }

    QString data = formatClientData(clientId, nom, prenom, pointsFidelite, remisePourcentage);
    writeData(data);

    qDebug() << "📤 Client envoyé à Arduino:" << data.trimmed();
    emit dataSent("Client: " + prenom + " " + nom);
}

void Arduino::displayWelcome()
{
    if (!isConnected()) return;

    writeData("ACCUEIL");
    qDebug() << "Accueil envoyé à Arduino";
}

void Arduino::displayMessage(const QString &line1, const QString &line2)
{
    if (!isConnected()) return;

    QString data = "MSG:" + line1 + "|" + line2;
    writeData(data);
    qDebug() << "Message envoyé à Arduino:" << data;
}

QString Arduino::getPortName() const
{
    return portName;
}

QString Arduino::getStatus() const
{
    if (!connectedState) return "Déconnecté";
    if (serialPort->isOpen()) return "Connecté sur " + portName;
    return "Erreur de connexion";
}

QString Arduino::findArduinoPort()
{
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    if (ports.isEmpty()) {
        qDebug() << "Aucun port série disponible";
        return "";
    }

    qDebug() << "Ports série disponibles:";
    for (const QSerialPortInfo &port : ports) {
        qDebug() << "  -" << port.portName()
        << "(" << port.description()
        << "-" << port.manufacturer() << ")";
    }

    // Priorité 1: Chercher "Arduino" dans la description
    for (const QSerialPortInfo &port : ports) {
        QString desc = port.description().toLower();
        QString manufacturer = port.manufacturer().toLower();

        if (desc.contains("arduino") || manufacturer.contains("arduino")) {
            qDebug() << "✅ Arduino identifié par nom:" << port.portName();
            return port.portName();
        }
    }

    // Priorité 2: Chercher CH340/FTDI (courant pour les clones Arduino)
    for (const QSerialPortInfo &port : ports) {
        QString desc = port.description().toLower();

        if (desc.contains("ch340") || desc.contains("ftdi") ||
            desc.contains("usb serial") || desc.contains("serial usb")) {
            qDebug() << "✅ Arduino identifié par contrôleur:" << port.portName();
            return port.portName();
        }
    }

    // Priorité 3: Prendre le premier port USB/COM (pour Windows/Linux)
    for (const QSerialPortInfo &port : ports) {
        QString portName = port.portName().toLower();

        if (portName.contains("com") || portName.contains("usb") ||
            portName.contains("acm") || portName.contains("ttyusb")) {
            qDebug() << "⚠️  Port série probable (à vérifier):" << port.portName();
            return port.portName();
        }
    }

    // Si rien trouvé, prendre le premier port
    if (!ports.isEmpty()) {
        qDebug() << "⚠️  Aucun Arduino identifié, utilisation du premier port:"
                 << ports.first().portName();
        return ports.first().portName();
    }

    return "";
}

void Arduino::startSensorMonitoring()
{
    if (!connectedState || !serialPort->isOpen()) {
        emit error("Arduino non connecté pour la surveillance du capteur");
        return;
    }

    monitoring = true;
    qDebug() << "🔍 Surveillance du capteur démarrée";
    emit statusChanged("Surveillance capteur active");
}

void Arduino::stopSensorMonitoring()
{
    monitoring = false;
    qDebug() << "🔍 Surveillance du capteur arrêtée";
}

bool Arduino::isMonitoring() const
{
    return monitoring;
}

void Arduino::readData()
{
    if (!monitoring || !serialPort->isOpen()) return;

    QByteArray data = serialPort->readAll();
    buffer.append(data);

    // Traiter les lignes complètes
    int index;
    while ((index = buffer.indexOf('\n')) != -1) {
        QString line = buffer.left(index).trimmed();
        buffer.remove(0, index + 1);

        if (!line.isEmpty()) {
            processSensorData(line);
        }
    }
}

void Arduino::processSensorData(const QString &data)
{
    qDebug() << "📡 Données capteur:" << data;

    if (data == "READY") {
        emit sensorReady();
        qDebug() << "✅ Capteur prêt";
    }
    else if (data == "1") {
        qDebug() << "🎯 OBJET DÉTECTÉ !";
        emit objectDetected();
    }
    else if (data == "0") {
        // Fente libre - rien à faire
        qDebug() << "📭 Fente libre";
    }
    else if (data.startsWith("ARDUINO_READY")) {
        qDebug() << "✅ Arduino initialisé et prêt";
    }
    else {
        qDebug() << "Données capteur reçues:" << data;
    }
}

void Arduino::writeData(const QString &data)
{
    if (!serialPort->isOpen()) {
        emit error("Port série non ouvert");
        return;
    }

    QByteArray byteData = (data + "\n").toUtf8();
    qint64 bytesWritten = serialPort->write(byteData);

    if (bytesWritten == -1) {
        emit error("Erreur d'écriture: " + serialPort->errorString());
    } else {
        serialPort->waitForBytesWritten(1000);
    }
}

QString Arduino::formatClientData(int clientId, const QString &nom, const QString &prenom,
                                  int points, int remise) const
{
    return QString("CLIENT:%1|NOM:%2|PRENOM:%3|POINTS:%4|REMISE:%5")
    .arg(clientId)
        .arg(nom)
        .arg(prenom)
        .arg(points)
        .arg(remise);
}

void Arduino::displaySale(int venteId, int clientId, const QString &nom, const QString &prenom,
                          int pointsFidelite, int remiseVente)
{
    if (!isConnected()) {
        emit error("Arduino non connecté");
        return;
    }

    // Format: VENTE:ID|CLIENT:ID|NOM:...|PRENOM:...|POINTS:...|REMISE_VENTE:...
    QString data = QString("VENTE:%1|CLIENT:%2|NOM:%3|PRENOM:%4|POINTS:%5|REMISE_VENTE:%6")
                       .arg(venteId)
                       .arg(clientId)
                       .arg(nom)
                       .arg(prenom)
                       .arg(pointsFidelite)
                       .arg(remiseVente);

    writeData(data);

    qDebug() << "📤 Vente envoyée à Arduino:" << data.trimmed();
    emit dataSent("Vente " + QString::number(venteId) + " - Client: " + prenom + " " + nom);
}
