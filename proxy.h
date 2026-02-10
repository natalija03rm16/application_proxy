#ifndef PROXY_H
#define PROXY_H
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QObject>

enum class ProxyState {
    Greeting,
    Auth,
    Request,
    Relay
};

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
    ProxyState proxyState;
};

#endif // PROXY_H
