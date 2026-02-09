#include "proxy.h"
#include <QDebug>

Proxy::Proxy(quint16 listenPort, const QString& serverHost_, quint16 serverPort_, QObject* parent)
    : QObject(parent), clientSocket(nullptr), serverSocket(nullptr),
    serverHost(serverHost_), serverPort(serverPort_)
{
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &Proxy::onNewClientConnection);

    if (!tcpServer->listen(QHostAddress::Any, listenPort)) {
        qCritical() << "Proxy could not start!";
    } else {
        qDebug() << "Proxy listening on port" << listenPort;
    }
}

void Proxy::onNewClientConnection()
{
    clientSocket = tcpServer->nextPendingConnection();
    qDebug() << "Client connected:" << clientSocket->peerAddress().toString();

    connect(clientSocket, &QTcpSocket::readyRead, this, &Proxy::onClientReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Proxy::onClientDisconnected);

    serverSocket = new QTcpSocket(this);
    connect(serverSocket, &QTcpSocket::readyRead, this, &Proxy::onServerReadyRead);
    connect(serverSocket, &QTcpSocket::disconnected, this, &Proxy::onServerDisconnected);

    serverSocket->connectToHost(serverHost, serverPort);
}

void Proxy::onClientReadyRead()
{
    QByteArray data = clientSocket->readAll();

    if (!clientAuthenticated) {
        QString received = QString::fromUtf8(data);
        QStringList parts = received.split(':');
        if (parts.size() == 2) {
            QString username = parts[0];
            QString password = parts[1];
            if (username == "user" && password == "pass") {
                clientAuthenticated = true;
                qDebug() << "Client authenticated:" << username;
            } else {
                qDebug() << "Client failed authentication:" << username;
                clientSocket->disconnectFromHost();
                return;
            }
        }
        return;
    }

    if (clientAuthenticated && serverSocket->state() == QTcpSocket::ConnectedState)
        serverSocket->write(data);
}

void Proxy::onServerReadyRead()
{
    QByteArray data = serverSocket->readAll();
    if (clientSocket && clientSocket->state() == QTcpSocket::ConnectedState)
        clientSocket->write(data);
}

void Proxy::onClientDisconnected()
{
    qDebug() << "Client disconnected";
    clientSocket->deleteLater();
    clientSocket = nullptr;
}

void Proxy::onServerDisconnected()
{
    qDebug() << "Server disconnected";
    serverSocket->deleteLater();
    serverSocket = nullptr;
}
