#include "server.h"
#include <QDebug>

Server::Server(quint16 port, QObject* parent)
    : QObject(parent)
{
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &Server::onNewConnection);

    if (!server->listen(QHostAddress::Any, port))
        qCritical() << "Server could not start!";
    else
        qDebug() << "Server started on port" << port;
}

void Server::onNewConnection()
{
    clientSocket = server->nextPendingConnection();
    qDebug() << "New client connected:" << clientSocket->peerAddress().toString();

    connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Server::onDisconnected);
}

void Server::onReadyRead()
{
    QByteArray data = clientSocket->readAll();
    qDebug() << "Server received:" << data;

    clientSocket->write("Server ACK: " + data);
}

void Server::onDisconnected()
{
    qDebug() << "Client disconnected";
    clientSocket->deleteLater();
    clientSocket = nullptr;
}
