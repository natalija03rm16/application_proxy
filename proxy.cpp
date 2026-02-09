#include "proxy.h"
#include <QDebug>

Proxy::Proxy(quint16 listenPort, const QString& serverHost_, quint16 serverPort_, QObject* parent)
    : QObject(parent),
    tcpServer(new QTcpServer(this)),
    clientSocket(nullptr),
    serverSocket(nullptr),
    serverHost(serverHost_),
    serverPort(serverPort_)
{
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

    // Konektujemo se na server
    serverSocket = new QTcpSocket(this);
    connect(serverSocket, &QTcpSocket::readyRead, this, &Proxy::onServerReadyRead);
    connect(serverSocket, &QTcpSocket::disconnected, this, &Proxy::onServerDisconnected);

    // Ako nije već konektovan
    if (serverSocket->state() != QTcpSocket::ConnectedState)
        serverSocket->connectToHost(serverHost, serverPort);
}

void Proxy::onClientReadyRead()
{
    if (!clientSocket) return; // sigurnosna provera
    QByteArray data = clientSocket->readAll();

    if (!clientAuthenticated) {
        // ---------------- SOCKS v5 HELLO ----------------
        if (data.size() >= 3 && static_cast<unsigned char>(data[0]) == 0x05) {
            QByteArray resp;
            resp.append(char(0x05)); // VER
            resp.append(char(0x02)); // METHOD = username/password
            clientSocket->write(resp);
            clientSocket->flush();
            return;
        }

        // ---------------- USERNAME/PASSWORD ----------------
        if (data.size() >= 5 && static_cast<unsigned char>(data[0]) == 0x01) {
            int ulen = static_cast<unsigned char>(data[1]);
            QString username = QString::fromUtf8(data.mid(2, ulen));
            int plen = static_cast<unsigned char>(data[2 + ulen]);
            QString password = QString::fromUtf8(data.mid(3 + ulen, plen));

            QByteArray authResp;
            authResp.append(char(0x01)); // VER
            if (username == "user" && password == "pass") {
                authResp.append(char(0x00)); // OK
                clientAuthenticated = true;
                qDebug() << "Client authenticated:" << username;
            } else {
                authResp.append(char(0x01)); // FAIL
                clientSocket->write(authResp);
                clientSocket->flush();
                clientSocket->disconnectFromHost();
                qDebug() << "Client failed authentication:" << username;
                return;
            }

            clientSocket->write(authResp);
            clientSocket->flush();
            return;
        }
    }

    // ---------------- Prosleđivanje podataka ----------------
    if (clientAuthenticated && serverSocket && serverSocket->state() == QTcpSocket::ConnectedState)
        serverSocket->write(data);
}

void Proxy::onServerReadyRead()
{
    if (!serverSocket) return;
    QByteArray data = serverSocket->readAll();
    qDebug() << "Proxy received from server:" << data;
    if (clientSocket && clientSocket->state() == QTcpSocket::ConnectedState)
        clientSocket->write(data);
}

void Proxy::onClientDisconnected()
{
    if (clientSocket) {
        qDebug() << "Client disconnected";
        clientSocket->deleteLater();
        clientSocket = nullptr;
        clientAuthenticated = false;
    }
}

void Proxy::onServerDisconnected()
{
    if (serverSocket) {
        qDebug() << "Server disconnected";
        serverSocket->deleteLater();
        serverSocket = nullptr;
    }
}
