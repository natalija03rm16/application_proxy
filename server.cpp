#include "server.h"
#include <QDebug>
#include <QDir>
#include <QFile>

Server::Server(quint16 port, QObject* parent)
    : QObject(parent), clientSocket(nullptr), receivedFile(nullptr), fileCounter(0)
{
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &Server::onNewConnection);

    if (!server->listen(QHostAddress::Any, port)) {
        qCritical() << "[SERVER] Could not start!";
    } else {
        qDebug() << "[SERVER] Listening on port" << port;
    }

    QDir dir("/home/natalija/Desktop/mrkirm/application_proxy/server_files");
    if (!dir.exists()) {
        dir.mkpath(".");
        qDebug() << "[SERVER] Created directory:" << dir.absolutePath();
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

    buffer.clear();

    qDebug() << "[SERVER] Ready to receive data...";
}

void Server::onReadyRead()
{
    if (!clientSocket)
        return;

    buffer.append(clientSocket->readAll());

    while (true) {
        if (buffer.startsWith("MSG|")) {
            int firstPipe = buffer.indexOf('|');
            int secondPipe = buffer.indexOf('|', firstPipe + 1);

            if (secondPipe == -1)
                break;

            QByteArray lenStr = buffer.mid(firstPipe + 1, secondPipe - firstPipe - 1);
            int msgLen = lenStr.toInt();

            if (buffer.size() < secondPipe + 1 + msgLen)
                break;

            QByteArray message = buffer.mid(secondPipe + 1, msgLen);

            qDebug() << "[SERVER] ----------------------------------------";
            qDebug() << "[SERVER] Received MESSAGE:";
            qDebug() << "[SERVER]   Size:" << msgLen << "bytes";
            qDebug() << "[SERVER]   Content:" << message;
            qDebug() << "[SERVER] ----------------------------------------";

            buffer.remove(0, secondPipe + 1 + msgLen);
        }
        else if (buffer.startsWith("FILE|")) {
            int firstPipe = buffer.indexOf('|');
            int secondPipe = buffer.indexOf('|', firstPipe + 1);

            if (secondPipe == -1)
                break;

            QByteArray fileName = buffer.mid(firstPipe + 1, secondPipe - firstPipe - 1);

            int thirdPipe = buffer.indexOf('|', secondPipe + 1);
            if (thirdPipe == -1)
                break;

            QByteArray lenStr = buffer.mid(secondPipe + 1, thirdPipe - secondPipe - 1);
            int fileLen = lenStr.toInt();

            if (buffer.size() < thirdPipe + 1 + fileLen)
                break;

            QByteArray fileData = buffer.mid(thirdPipe + 1, fileLen);

            QString filePath = "/home/natalija/Desktop/mrkirm/application_proxy/server_files/" + QString::fromUtf8(fileName);
            QFile file(filePath);

            if (file.open(QIODevice::WriteOnly)) {
                file.write(fileData);
                file.close();

                qDebug() << "[SERVER] ========================================";
                qDebug() << "[SERVER] Received FILE:";
                qDebug() << "[SERVER]   Name:" << fileName;
                qDebug() << "[SERVER]   Size:" << fileLen << "bytes";
                qDebug() << "[SERVER]   Saved to:" << filePath;
                qDebug() << "[SERVER] ========================================";
            } else {
                qCritical() << "[SERVER] Failed to save file:" << filePath;
            }
            buffer.remove(0, thirdPipe + 1 + fileLen);
        }
        else {
            break;
        }
    }
}

void Server::onDisconnected()
{
    qDebug() << "[SERVER] ========================================";
    qDebug() << "[SERVER] Client disconnected";
    qDebug() << "[SERVER] Cleaning up connection...";
    qDebug() << "[SERVER] ========================================";

    if (clientSocket) {
        clientSocket->disconnect();
        clientSocket->deleteLater();
        clientSocket = nullptr;
    }

    buffer.clear();

    qDebug() << "[SERVER] Ready for new connections...";
}
