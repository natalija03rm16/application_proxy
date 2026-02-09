#include "server.h"
#include <QDebug>

Server::Server(quint16 port, QObject* parent) : QObject(parent)
{
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &Server::onNewConnection);

    if (!tcpServer->listen(QHostAddress::Any, port)) {
        qCritical() << "Server could not start!";
    } else {
        qDebug() << "Server started on port" << port;
    }
}

void Server::onNewConnection()
{
    QTcpSocket* clientSocket = tcpServer->nextPendingConnection();
    qDebug() << "New client connected:" << clientSocket->peerAddress().toString();

    connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
}

void Server::onReadyRead()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());
    QByteArray data = clientSocket->readAll();
    qDebug() << "Server received:" << data;

    // Echo nazad
    clientSocket->write("Server ACK: " + data);
}
