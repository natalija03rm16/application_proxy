#include "server.h"
#include <QDebug>

Server::Server(quint16 port, QObject* parent)
    : QObject(parent)
{
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &Server::onNewConnection);

    if (!server->listen(QHostAddress::Any, port)) {
        qCritical() << "[SERVER] Could not start!";
    } else {
        qDebug() << "[SERVER] Started and listening on port" << port;
        qDebug() << "[SERVER] Waiting for connections...";
    }
}

void Server::onNewConnection()
{
    clientSocket = server->nextPendingConnection();
    qDebug() << "[SERVER] ========================================";
    qDebug() << "[SERVER] New client connected!";
    qDebug() << "[SERVER] Client address:" << clientSocket->peerAddress().toString();
    qDebug() << "[SERVER] Client port:" << clientSocket->peerPort();
    qDebug() << "[SERVER] ========================================";

    connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Server::onDisconnected);

    qDebug() << "[SERVER] Ready to receive data...";
}

void Server::onReadyRead()
{
    QByteArray data = clientSocket->readAll();

    qDebug() << "[SERVER] ----------------------------------------";
    qDebug() << "[SERVER] Received data from client:";
    qDebug() << "[SERVER]   Size:" << data.size() << "bytes";
    qDebug() << "[SERVER]   Content:" << data;
    qDebug() << "[SERVER] ----------------------------------------";

    // Prepare response
    QByteArray response = "Server ACK: " + data;

    qDebug() << "[SERVER] Sending response to client:";
    qDebug() << "[SERVER]   Size:" << response.size() << "bytes";
    qDebug() << "[SERVER]   Content:" << response;

    clientSocket->write(response);

    qDebug() << "[SERVER] Response sent successfully!";
}

void Server::onDisconnected()
{
    qDebug() << "[SERVER] ========================================";
    qDebug() << "[SERVER] Client disconnected";
    qDebug() << "[SERVER] Cleaning up connection...";
    qDebug() << "[SERVER] ========================================";

    clientSocket->deleteLater();
    clientSocket = nullptr;

    qDebug() << "[SERVER] Ready for new connections...";
}
