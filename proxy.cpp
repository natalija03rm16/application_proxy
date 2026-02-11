#include "proxy.h"
#include <QDebug>
#include <QByteArray>

Proxy::Proxy(quint16 listenPort, const QString& serverHost_, quint16 serverPort_, QObject* parent)
    : QObject(parent), clientSocket(nullptr), serverSocket(nullptr),
    serverHost(serverHost_), serverPort(serverPort_), proxyState(ProxyState::Greeting)
{
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &Proxy::onNewClientConnection);

    if (!tcpServer->listen(QHostAddress::Any, listenPort)) {
        qCritical() << "[PROXY] Could not start!";
    } else {
        qDebug() << "[PROXY] Listening on port" << listenPort;
    }
}

void Proxy::onNewClientConnection()
{
    clientSocket = tcpServer->nextPendingConnection();
    qDebug() << "[PROXY] Client connected:" << clientSocket->peerAddress().toString();

    connect(clientSocket, &QTcpSocket::readyRead, this, &Proxy::onClientReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &Proxy::onClientDisconnected);

    // reset state - new client
    proxyState = ProxyState::Greeting;
    qDebug() << "[PROXY] Waiting for SOCKS5 Greeting...";
}

void Proxy::onClientReadyRead()
{
    QByteArray data = clientSocket->readAll();

    switch (proxyState) {
    case ProxyState::Greeting: {
        qDebug() << "[PROXY] Received SOCKS5 Greeting:" << data.size() << "bytes";
        qDebug() << "[PROXY] Greeting: VER=" << (quint8)data[0]
                 << "NMETHODS=" << (quint8)data[1];

        if (data.size() >= 3) {
            qDebug() << "[PROXY] Client supports method:" << (quint8)data[2];
        }

        QByteArray resp;
        resp.append(char(0x05));
        resp.append(char(0x02)); // username/password

        qDebug() << "[PROXY] Sending Greeting Response: VER=0x05, METHOD=0x02 (Username/Password)";
        clientSocket->write(resp);
        proxyState = ProxyState::Auth;
        qDebug() << "[PROXY] Waiting for Authentication...";
        break;
    }

    case ProxyState::Auth: {
        qDebug() << "[PROXY] Received Authentication:" << data.size() << "bytes";

        if (data.size() < 3) {
            qCritical() << "[PROXY] Invalid auth packet!";
            clientSocket->disconnectFromHost();
            return;
        }

        quint8 ulen = static_cast<quint8>(data[1]);
        QString user = QString::fromUtf8(data.mid(2, ulen));

        if (data.size() < 3 + ulen) {
            qCritical() << "[PROXY] Invalid auth packet - missing password!";
            clientSocket->disconnectFromHost();
            return;
        }

        quint8 plen = static_cast<quint8>(data[2 + ulen]);
        QString pass = QString::fromUtf8(data.mid(3 + ulen, plen));

        qDebug() << "[PROXY] Auth VER=" << (quint8)data[0]
                 << "USERNAME=" << user;

        QByteArray resp;
        resp.append(char(0x01));

        if (user == "user" && pass == "pass") {
            resp.append(char(0x00));
            qDebug() << "[PROXY] Authentication SUCCESS! STATUS=0x00";
            proxyState = ProxyState::Request;
            qDebug() << "[PROXY] Waiting for CONNECT Request...";
        } else {
            resp.append(char(0x01));
            qCritical() << "[PROXY] Authentication FAILED! Invalid credentials. STATUS=0x01";
            clientSocket->write(resp);
            clientSocket->disconnectFromHost();
            return;
        }
        clientSocket->write(resp);
        break;
    }

    case ProxyState::Request: {

        if (data.size() < 10) {
            qCritical() << "[PROXY] Invalid CONNECT request!";
            clientSocket->disconnectFromHost();
            return;
        }

        quint8 ver  = static_cast<quint8>(data[0]);
        quint8 cmd  = static_cast<quint8>(data[1]);
        quint8 atyp = static_cast<quint8>(data[3]);

        if (ver != 0x05 || cmd != 0x01) {
            qCritical() << "[PROXY] Unsupported request!";
            clientSocket->disconnectFromHost();
            return;
        }

        QString dstAddress;
        quint16 dstPort;

        int index = 4;

        // ipv4
        if (atyp == 0x01) {

            quint32 ip =
                (static_cast<quint8>(data[index]) << 24) |
                (static_cast<quint8>(data[index+1]) << 16) |
                (static_cast<quint8>(data[index+2]) << 8)  |
                (static_cast<quint8>(data[index+3]));

            QHostAddress addr(ip);
            dstAddress = addr.toString();

            index += 4;
        }

        // domain name
        else if (atyp == 0x03) {

            quint8 len = static_cast<quint8>(data[index]);
            index += 1;

            dstAddress = QString::fromUtf8(data.mid(index, len));
            index += len;

        }
        else {
            qCritical() << "[PROXY] Unsupported ATYP!";
            clientSocket->disconnectFromHost();
            return;
        }

        // port
        dstPort = (static_cast<quint8>(data[index]) << 8) |
                  static_cast<quint8>(data[index+1]);

        qDebug() << "[PROXY] CONNECT to:" << dstAddress << ":" << dstPort;

        // real connection to requested host
        serverSocket = new QTcpSocket(this);
        connect(serverSocket, &QTcpSocket::readyRead, this, &Proxy::onServerReadyRead);
        connect(serverSocket, &QTcpSocket::disconnected, this, &Proxy::onServerDisconnected);

        serverSocket->connectToHost(dstAddress, dstPort);

        if (!serverSocket->waitForConnected(3000)) {
            qCritical() << "[PROXY] Failed to connect to destination!";

            QByteArray failResp;
            failResp.append(char(0x05));
            failResp.append(char(0x05)); // connection refused
            failResp.append(char(0x00));
            failResp.append(char(0x01));
            failResp.append(QByteArray(6, char(0x00)));

            clientSocket->write(failResp);
            clientSocket->disconnectFromHost();
            return;
        }

        // succes resp
        QByteArray resp;
        resp.append(char(0x05));
        resp.append(char(0x00)); // success
        resp.append(char(0x00));
        resp.append(char(0x01));
        resp.append(QByteArray(6, char(0x00)));

        clientSocket->write(resp);

        proxyState = ProxyState::Relay;
        qDebug() << "[PROXY] Tunnel established - RELAY mode";

        break;
    }

    case ProxyState::Relay:
        qDebug() << "[PROXY] [RELAY] Client -> Server:" << data.size() << "bytes";
        if (serverSocket)
            serverSocket->write(data);
        break;
    }
}

void Proxy::onServerReadyRead()
{
    QByteArray data = serverSocket->readAll();
    qDebug() << "[PROXY] [RELAY] Server -> Client:" << data.size() << "bytes";

    if (clientSocket && clientSocket->state() == QTcpSocket::ConnectedState)
        clientSocket->write(data);
}

void Proxy::onClientDisconnected()
{
    qDebug() << "[PROXY] Client disconnected";

    if (clientSocket) {
        clientSocket->deleteLater();
        clientSocket = nullptr;
    }

    proxyState = ProxyState::Greeting;
    qDebug() << "[PROXY] Client socket cleaned up. Server connection still active.";
}


void Proxy::onServerDisconnected()
{
    qDebug() << "[PROXY] Server disconnected";

    if (serverSocket) {
        serverSocket->deleteLater();
        serverSocket = nullptr;
    }

    proxyState = ProxyState::Greeting;
    qDebug() << "[PROXY] Server socket cleaned up. Client connection still active.";
}

