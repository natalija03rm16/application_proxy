#include "proxy.h"
#include "authenticate.h"
#include <QDebug>
#include <QByteArray>

Proxy::Proxy(quint16 listenPort, QObject* parent) : QObject(parent)
{
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &Proxy::onNewClientConnection);

    if (!tcpServer->listen(QHostAddress::Any, listenPort))
        qCritical() << "[PROXY] Could not start!";
    else
    {
        qDebug() << "[PROXY] Listening on port" << listenPort;
        qDebug() << "[PROXY] Maximum clients allowed:" << MAX_CLIENTS;
    }
}

void Proxy::onNewClientConnection()
{
    if (clients.size() >= MAX_CLIENTS)
    {
        QTcpSocket* tmp = tcpServer->nextPendingConnection();
        qCritical() << "[PROXY] ========================================";
        qCritical() << "[PROXY] Max clients reached (" << MAX_CLIENTS << "). Rejecting new connection.";
        qCritical() << "[PROXY] Rejected client:" << tmp->peerAddress().toString();
        qCritical() << "[PROXY] ========================================";
        tmp->disconnectFromHost();
        tmp->deleteLater();
        return;
    }

    QTcpSocket* socket = tcpServer->nextPendingConnection();

    qDebug() << "[PROXY] ========================================";
    qDebug() << "[PROXY] New client connected!";
    qDebug() << "[PROXY] Client address:" << socket->peerAddress().toString();
    qDebug() << "[PROXY] Client port:" << socket->peerPort();
    qDebug() << "[PROXY] Total clients:" << (clients.size() + 1);
    qDebug() << "[PROXY] ========================================";

    ClientContext ctx;
    ctx.clientSocket = socket;
    ctx.serverSocket = nullptr;
    ctx.state = ProxyState::Greeting;

    clients.insert(socket, ctx);

    connect(socket, &QTcpSocket::readyRead, this, &Proxy::onClientReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &Proxy::onClientDisconnected);

    qDebug() << "[PROXY] Waiting for SOCKS5 Greeting from" << socket->peerAddress().toString();
}

void Proxy::onClientReadyRead()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());

    if (!clients.contains(socket))
    {
        qWarning() << "[PROXY] Received data from unknown client!";
        return;
    }

    ClientContext& ctx = clients[socket];
    QByteArray data = socket->readAll();

    QString clientAddr = socket->peerAddress().toString();

    switch (ctx.state) {
    case ProxyState::Greeting:
    {
        qDebug() << "[PROXY] ----------------------------------------";
        qDebug() << "[PROXY] Received SOCKS5 Greeting from" << clientAddr;
        qDebug() << "[PROXY] Data size:" << data.size() << "bytes";

        if (data.size() >= 2)
        {
            qDebug() << "[PROXY] VER:" << (quint8)data[0];
            qDebug() << "[PROXY] NMETHODS:" << (quint8)data[1];
            if (data.size() >= 3)
                qDebug() << "[PROXY] METHOD:" << (quint8)data[2];
        }

        qDebug() << "[PROXY] Sending Greeting Response: VER=0x05, METHOD=0x02 (Username/Password)";
        socket->write(QByteArray("\x05\x02", 2));
        ctx.state = ProxyState::Auth;
        qDebug() << "[PROXY] State changed to: Auth";
        qDebug() << "[PROXY] Waiting for Authentication from" << clientAddr;
        qDebug() << "[PROXY] ----------------------------------------";
        break;
    }

    case ProxyState::Auth:
    {
        qDebug() << "[PROXY] ----------------------------------------";
        qDebug() << "[PROXY] Received Authentication from" << clientAddr;
        qDebug() << "[PROXY] Data size:" << data.size() << "bytes";

        // parse username and password
        if (data.size() >= 3) {
            quint8 ulen = static_cast<quint8>(data[1]);
            QString user = QString::fromUtf8(data.mid(2, ulen));

            if (data.size() >= 3 + ulen)
            {
                quint8 plen = static_cast<quint8>(data[2 + ulen]);
                QString pass = QString::fromUtf8(data.mid(3 + ulen, plen));

                qDebug() << "[PROXY] Auth VER:" << (quint8)data[0];
                qDebug() << "[PROXY] USERNAME:" << user;

                Authenticate auth;

                if (auth.login(user, pass))
                {
                    qDebug() << "[PROXY] Authentication SUCCESS! STATUS=0x00";
                    socket->write(QByteArray("\x01\x00", 2));
                    ctx.state = ProxyState::Request;
                    qDebug() << "[PROXY] State changed to: Request";
                    qDebug() << "[PROXY] Waiting for CONNECT Request from" << clientAddr;
                }
                else
                {
                    qCritical() << "[PROXY] Authentication FAILED! Invalid credentials. STATUS=0x01";
                    socket->write(QByteArray("\x01\x01", 2));
                    socket->disconnectFromHost();
                    qDebug() << "[PROXY] Disconnecting client due to failed auth";
                }
            }
            else
            {
                qCritical() << "[PROXY] Invalid auth packet - missing password!";
                socket->disconnectFromHost();
            }
        }
        else
        {
            qCritical() << "[PROXY] Invalid auth packet - too short!";
            socket->disconnectFromHost();
        }

        qDebug() << "[PROXY] ----------------------------------------";
        break;
    }

    case ProxyState::Request:
    {
        qDebug() << "[PROXY] ----------------------------------------";
        qDebug() << "[PROXY] Received CONNECT Request from" << clientAddr;
        qDebug() << "[PROXY] Data size:" << data.size() << "bytes";

        if (data.size() >= 4)
        {
            qDebug() << "[PROXY] VER:" << (quint8)data[0];
            qDebug() << "[PROXY] CMD:" << (quint8)data[1];
            qDebug() << "[PROXY] ATYP:" << (quint8)data[3];
        }

        QString dstAddress = "127.0.0.1";
        quint16 dstPort = 12345;

        qDebug() << "[PROXY] Opening connection to server" << dstAddress << ":" << dstPort;

        ctx.serverSocket = new QTcpSocket(this);
        connect(ctx.serverSocket, &QTcpSocket::readyRead, this, &Proxy::onServerReadyRead);
        connect(ctx.serverSocket, &QTcpSocket::disconnected, this, &Proxy::onServerDisconnected);

        ctx.serverSocket->connectToHost(dstAddress, dstPort);

        QByteArray resp;
        resp.append(char(0x05));  // VER
        resp.append(char(0x00));  // REP = success
        resp.append(char(0x00));  // RSV
        resp.append(char(0x01));  // ATYP = IPv4
        resp.append(QByteArray(6, char(0x00)));  // BND.ADDR + BND.PORT

        qDebug() << "[PROXY] Sending CONNECT Response: REP=0x00 (Success)";
        socket->write(resp);

        ctx.state = ProxyState::Relay;
        qDebug() << "[PROXY] State changed to: Relay";
        qDebug() << "[PROXY] *** Entering RELAY mode for" << clientAddr << "***";
        qDebug() << "[PROXY] ----------------------------------------";
        break;
    }

    case ProxyState::Relay:
    {
        QString clientInfo = clientAddr + ":" + QString::number(socket->peerPort());
        qDebug() << "[PROXY] [RELAY] Client -> Server:" << data.size() << "bytes from" << clientInfo;

        if (ctx.serverSocket && ctx.serverSocket->state() == QTcpSocket::ConnectedState)
        {
            ctx.serverSocket->write(data);
            qDebug() << "[PROXY] [RELAY] Data forwarded to server";
        }
        else
            qWarning() << "[PROXY] [RELAY] Server socket not connected! Cannot forward data.";

        break;
    }
    }
}

void Proxy::onServerReadyRead()
{
    QTcpSocket* server = qobject_cast<QTcpSocket*>(sender());

    for (auto it = clients.begin(); it != clients.end(); ++it)
    {
        if (it->serverSocket == server)
        {
            QByteArray data = server->readAll();

            QString clientAddr = it->clientSocket->peerAddress().toString();
            QString clientInfo = clientAddr + ":" + QString::number(it->clientSocket->peerPort());

            qDebug() << "[PROXY] [RELAY] Server -> Client:" << data.size() << "bytes to" << clientInfo;

            if (it->clientSocket && it->clientSocket->state() == QTcpSocket::ConnectedState)
            {
                it->clientSocket->write(data);
                qDebug() << "[PROXY] [RELAY] Data forwarded to client";
            }
            else
                qWarning() << "[PROXY] [RELAY] Client socket not connected! Cannot forward data.";

            break;
        }
    }
}

void Proxy::onClientDisconnected()
{
    QTcpSocket* socket = qobject_cast<QTcpSocket*>(sender());

    if (!clients.contains(socket))
    {
        qWarning() << "[PROXY] Unknown client disconnected!";
        return;
    }

    QString clientAddr = socket->peerAddress().toString();

    qDebug() << "[PROXY] ========================================";
    qDebug() << "[PROXY] Client disconnected:" << clientAddr + ":" + QString::number(socket->peerPort());
    qDebug() << "[PROXY] Cleaning up connections...";

    ClientContext ctx = clients.take(socket);

    if (ctx.serverSocket)
    {
        qDebug() << "[PROXY] Closing server connection...";
        ctx.serverSocket->disconnect();
        ctx.serverSocket->disconnectFromHost();
        ctx.serverSocket->deleteLater();
    }

    socket->disconnect();
    socket->deleteLater();

    qDebug() << "[PROXY] Total clients remaining:" << clients.size();
    qDebug() << "[PROXY] ========================================";
}

void Proxy::onServerDisconnected()
{
    QTcpSocket* server = qobject_cast<QTcpSocket*>(sender());

    qDebug() << "[PROXY] ========================================";
    qDebug() << "[PROXY] Server disconnected";

    for (auto it = clients.begin(); it != clients.end(); ++it)
    {
        if (it.value().serverSocket == server)
        {
            QString clientAddr = it.value().clientSocket->peerAddress().toString();
            qDebug() << "[PROXY] Server disconnected for client:" << clientAddr;
            qDebug() << "[PROXY] Cleaning up server socket...";

            server->disconnect();
            server->deleteLater();
            it.value().serverSocket = nullptr;

            qDebug() << "[PROXY] Server socket cleaned up. Client connection still active.";
            break;
        }
    }

    qDebug() << "[PROXY] ========================================";
}
