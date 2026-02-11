#ifndef PROXY_H
#define PROXY_H

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QObject>
#include <QMap>

enum class ProxyState
{
    Greeting,
    Auth,
    Request,
    Relay
};

struct ClientContext
{
    QTcpSocket* clientSocket;
    QTcpSocket* serverSocket;
    ProxyState state;
};

class Proxy : public QObject
{
    Q_OBJECT
public:
    Proxy(quint16 listenPort, QObject* parent = nullptr);

private slots:
    void onNewClientConnection();
    void onClientReadyRead();
    void onServerReadyRead();
    void onClientDisconnected();
    void onServerDisconnected();

private:
    QTcpServer* tcpServer;

    QMap<QTcpSocket*, ClientContext> clients;

    const int MAX_CLIENTS = 10;
};

#endif
