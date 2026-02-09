#ifndef CLIENT_H
#define CLIENT_H

#include <QtNetwork/QTcpSocket>
#include <QObject>

class Client : public QObject
{
    Q_OBJECT
public:
    // <-- SAMO JEDAN KONSTRUKTOR
    Client(const QString& proxyHost, quint16 proxyPort, QObject* parent = nullptr);

    void sendMessage(const QString& message);  // public sada

private slots:
    void onReadyRead();
    void onConnected();

private:
    QTcpSocket* socket;
    QString username;
    QString password;

    bool authStep1Done = false;
    bool authStep2Done = false;

};

#endif // CLIENT_H
