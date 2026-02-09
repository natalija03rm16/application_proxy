#ifndef PROXY_H
#define PROXY_H

#include <QObject>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

class Proxy : public QObject
{
    Q_OBJECT
public:
    explicit Proxy(quint16 listenPort,
                   const QString& serverHost,
                   quint16 serverPort,
                   QObject* parent = nullptr);

private slots:
    void onNewClientConnection();
    void onClientReadyRead();
    void onServerReadyRead();
    void onClientDisconnected();
    void onServerDisconnected();

private:
    QTcpSocket* socket;
    QString username;
    QString password;

    QTcpServer* tcpServer;
    QTcpSocket* clientSocket;
    QTcpSocket* serverSocket;

    QString serverHost;
    quint16 serverPort;

    bool clientAuthenticated = false;  // Dodato da ne bude undefined
};

#endif // PROXY_H
