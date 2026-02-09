#ifndef PROXY_H
#define PROXY_H

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QObject>

class Proxy : public QObject
{
    Q_OBJECT
public:
    Proxy(quint16 listenPort, const QString& serverHost, quint16 serverPort, QObject* parent = nullptr);

private slots:
    void onNewClientConnection();
    void onClientReadyRead();
    void onServerReadyRead();
    void onClientDisconnected();
    void onServerDisconnected();

private:
    QTcpServer* tcpServer;
    QTcpSocket* clientSocket;
    QTcpSocket* serverSocket;

    QString serverHost;
    quint16 serverPort;
    bool clientAuthenticated = false;
};

#endif // PROXY_H
