#include "server.h"
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

Server::Server(quint16 port, QObject* parent) : QObject(parent), fileCounter(0)
{
    server = new QTcpServer(this);
    connect(server, &QTcpServer::newConnection, this, &Server::onNewConnection);

    if (!server->listen(QHostAddress::Any, port))
        qCritical() << "[SERVER] Could not start!";
    else
        qDebug() << "[SERVER] Listening on port" << port;
}

void Server::onNewConnection()
{
    QTcpSocket* clientSocket = server->nextPendingConnection();

    qDebug() << "[SERVER] ========================================";
    qDebug() << "[SERVER] New client connected!";
    qDebug() << "[SERVER] Client address:" << clientSocket->peerAddress().toString();
    qDebug() << "[SERVER] Client port:" << clientSocket->peerPort();
    qDebug() << "[SERVER] Total clients:" << (clients.size() + 1);
    qDebug() << "[SERVER] ========================================";

    // make a context for a client
    ServerClientContext ctx;
    ctx.socket = clientSocket;
    ctx.buffer.clear();

    // add to map
    clients.insert(clientSocket, ctx);

    connect(clientSocket, &QTcpSocket::readyRead, this, &Server::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Server::onDisconnected);

    qDebug() << "[SERVER] Ready to receive data from this client...";
}

void Server::onReadyRead()
{
    // identify a client
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());

    if (!clients.contains(clientSocket)) {
        qWarning() << "[SERVER] Received data from unknown client!";
        return;
    }

    // get client context
    ServerClientContext& ctx = clients[clientSocket];

    // add data to client buffer
    ctx.buffer.append(clientSocket->readAll());

    QString clientAddr = clientSocket->peerAddress().toString();
    QString clientInf = clientAddr + ":" + QString::number(clientSocket->peerPort());
    qDebug() << "[SERVER] Received data from" << clientInf << "- buffer size:" << ctx.buffer.size() << "bytes";

    // parse buffer data
    while (true) {
        // is it msg packet
        if (ctx.buffer.startsWith("MSG|"))
        {
            int firstPipe = ctx.buffer.indexOf('|');
            int secondPipe = ctx.buffer.indexOf('|', firstPipe + 1);

            if (secondPipe == -1)
                break; // not enough data

            // extract message size
            QByteArray lenStr = ctx.buffer.mid(firstPipe + 1, secondPipe - firstPipe - 1);
            int msgLen = lenStr.toInt();

            // is the message whole
            if (ctx.buffer.size() < secondPipe + 1 + msgLen)
                break; // not enough data

            // extract message
            QByteArray message = ctx.buffer.mid(secondPipe + 1, msgLen);
            qDebug() << "[SERVER] ----------------------------------------";
            qDebug() << "[SERVER] Received MESSAGE from" << clientSocket->peerAddress().toString() + ":" + QString::number(clientSocket->peerPort());
            qDebug() << "[SERVER]   Size:" << msgLen << "bytes";
            qDebug() << "[SERVER]   Content:" << message;
            qDebug() << "[SERVER] ----------------------------------------";

            // remove packet form buffer
            ctx.buffer.remove(0, secondPipe + 1 + msgLen);
        }
        // is a file packet
        else if (ctx.buffer.startsWith("FILE|"))
        {
            int firstPipe = ctx.buffer.indexOf('|');
            int secondPipe = ctx.buffer.indexOf('|', firstPipe + 1);

            if (secondPipe == -1)
                break; // not enough data

            // file name
            QByteArray fileName = ctx.buffer.mid(firstPipe + 1, secondPipe - firstPipe - 1);

            int thirdPipe = ctx.buffer.indexOf('|', secondPipe + 1);
            if (thirdPipe == -1)
                break; // not enough data

            // file size
            QByteArray lenStr = ctx.buffer.mid(secondPipe + 1, thirdPipe - secondPipe - 1);
            int fileLen = lenStr.toInt();

            qDebug() << "[SERVER] File header parsed: name=" << fileName << ", expected size=" << fileLen;
            qDebug() << "[SERVER] Buffer has:" << ctx.buffer.size() << "bytes, need:" << (thirdPipe + 1 + fileLen);

            // is the file whole
            if (ctx.buffer.size() < thirdPipe + 1 + fileLen)
            {
                qDebug() << "[SERVER] Waiting for more file data...";
                break; // not enough data
            }

            // extract file data
            QByteArray fileData = ctx.buffer.mid(thirdPipe + 1, fileLen);

            // save file localy
            QString filePath = "/home/natalija/Desktop/mrkirm/application_proxy/server_files/" + QString::fromUtf8(fileName);
            QFile file(filePath);

            if (file.open(QIODevice::WriteOnly))
            {
                qint64 written = file.write(fileData);
                file.close();

                qDebug() << "[SERVER] ========================================";
                qDebug() << "[SERVER] Received FILE from" << clientSocket->peerAddress().toString();
                qDebug() << "[SERVER]   Name:" << fileName;
                qDebug() << "[SERVER]   Expected size:" << fileLen << "bytes";
                qDebug() << "[SERVER]   Written size:" << written << "bytes";
                qDebug() << "[SERVER]   Saved to:" << filePath;
                qDebug() << "[SERVER] ========================================";

                // client ack
                QByteArray ack = "ACK|File received: " + fileName;
                clientSocket->write(ack);
                clientSocket->flush();

            }
            else
                qCritical() << "[SERVER] Failed to save file:" << filePath;

            // removing packet from buffer
            ctx.buffer.remove(0, thirdPipe + 1 + fileLen);
        }
        else
        {
            break;
        }
    }
}

void Server::onDisconnected()
{
    QTcpSocket* clientSocket = qobject_cast<QTcpSocket*>(sender());

    if (!clients.contains(clientSocket))
    {
        qWarning() << "[SERVER] Unknown client disconnected!";
        return;
    }

    qDebug() << "[SERVER] ========================================";
    qDebug() << "[SERVER] Client disconnected:" << clientSocket->peerAddress().toString() + ":" + QString::number(clientSocket->peerPort());
    qDebug() << "[SERVER] Cleaning up connection...";
    qDebug() << "[SERVER] ========================================";

    // remove from map
    clients.remove(clientSocket);

    // cleanup
    clientSocket->disconnect();
    clientSocket->deleteLater();

    qDebug() << "[SERVER] Total clients:" << clients.size();
    qDebug() << "[SERVER] Ready for new connections...";
}
