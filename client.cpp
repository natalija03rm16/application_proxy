#include "client.h"
#include <QInputDialog>
#include <QLineEdit>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QCoreApplication>

Client::Client(const QString& proxyHost, quint16 proxyPort, QObject* parent) : QObject(parent), clientState(ClientState::Greeting)
{
    this->username = QInputDialog::getText(nullptr, "Login", "Username:").trimmed();
    this->password = QInputDialog::getText(nullptr, "Login", "Password:", QLineEdit::Password).trimmed();

    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &Client::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &Client::onDisconnected);

    qDebug() << "[CLIENT] Connecting to proxy at" << proxyHost << ":" << proxyPort;
    socket->connectToHost(proxyHost, proxyPort);
}

Client::~Client()
{
    if (socket)
    {
        socket->disconnect();
        socket->deleteLater();
        socket = nullptr;
    }
}

void Client::onConnected()
{
    qDebug() << "[CLIENT] Connected to proxy";

    QByteArray greeting;
    greeting.append(char(0x05)); // VER
    greeting.append(char(0x01)); // NMETHODS
    greeting.append(char(0x02)); // USERNAME/PASSWORD

    qDebug() << "[CLIENT] Sending SOCKS5 Greeting: VER=0x05, NMETHODS=1, METHOD=0x02 (Username/Password)";
    socket->write(greeting);
}

void Client::sendMessage(const QString& message)
{
    if (!socket || socket->state() != QTcpSocket::ConnectedState)
    {
        qCritical() << "[CLIENT] Cannot send message: not connected";
        return;
    }

    // MSG|<len>|<message>
    QByteArray packet;
    packet.append("MSG|");

    QByteArray msgData = message.toUtf8();
    packet.append(QString::number(msgData.size()).toUtf8());
    packet.append("|");
    packet.append(msgData);

    qDebug() << "[CLIENT] Sending message:" << message;
    socket->write(packet);
    socket->flush();
}

void Client::askAndSendFile()
{
    while (true)
    {
        QString filePath = QInputDialog::getText(nullptr, "Send File", "Enter path to file (or leave empty to quit):");
        if (filePath.isEmpty())
        {
            qDebug() << "[CLIENT] No file entered. You can continue sending messages or files later.";
            break;
        }

        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            qCritical() << "[CLIENT] Cannot open file:" << filePath;
            continue;
        }

        QFileInfo fileInfo(filePath);
        QByteArray fileData = file.readAll();
        file.close();

        QByteArray packet;
        packet.append("FILE|");
        packet.append(fileInfo.fileName().toUtf8());
        packet.append("|");
        packet.append(QString::number(fileData.size()).toUtf8());
        packet.append("|");
        packet.append(fileData);

        socket->write(packet);
        socket->flush();

        qDebug() << "[CLIENT] File sent:" << fileInfo.fileName() << ", size:" << fileData.size() << "bytes";
    }
}

void Client::onReadyRead()
{
    QByteArray data = socket->readAll();

    switch (clientState)
    {
    case ClientState::Greeting:
    {
        qDebug() << "[CLIENT] Received Greeting Response:" << data.size() << "bytes";

        if (data.size() != 2 || data[1] != char(0x02))
        {
            qCritical() << "[CLIENT] SOCKS greeting rejected! VER=" << (quint8)data[0] << "METHOD=" << (quint8)data[1];
            socket->disconnectFromHost();
            return;
        }

        qDebug() << "[CLIENT] Greeting OK: VER=" << (quint8)data[0] << "METHOD=" << (quint8)data[1] << "(Username/Password selected)";

        QByteArray auth;
        QByteArray user = username.toUtf8();
        QByteArray pass = password.toUtf8();

        auth.append(char(0x01));                // auth version
        auth.append(char(user.size()));         // ULEN
        auth.append(user);
        auth.append(char(pass.size()));         // PLEN
        auth.append(pass);

        qDebug() << "[CLIENT] Sending Authentication: USERNAME=" << user;
        socket->write(auth);
        clientState = ClientState::Auth;
        break;
    }

    case ClientState::Auth:
    {
        qDebug() << "[CLIENT] Received Auth Response:" << data.size() << "bytes";

        if (data.size() != 2 || data[1] != char(0x00))
        {
            qCritical() << "[CLIENT] Authentication failed! STATUS=" << (quint8)data[1];
            socket->disconnectFromHost();
            clientState = ClientState::Auth;
            return;
        }

        qDebug() << "[CLIENT] Authentication OK: VER=" << (quint8)data[0] << "STATUS=" << (quint8)data[1] << "(Success)";

        QByteArray req;
        req.append(char(0x05)); // VER
        req.append(char(0x01)); // CMD = CONNECT
        req.append(char(0x00)); // RSV
        req.append(char(0x01)); // ATYP = IPv4

        quint32 ip = QHostAddress("127.0.0.1").toIPv4Address();
        req.append(char(ip >> 24));
        req.append(char(ip >> 16));
        req.append(char(ip >> 8));
        req.append(char(ip));

        req.append(char(0x30));
        req.append(char(0x39));

        qDebug() << "[CLIENT] Sending CONNECT Request...";
        socket->write(req);
        clientState = ClientState::Request;
        break;
    }

    case ClientState::Request:
    {
        qDebug() << "[CLIENT] Received CONNECT Response:" << data.size() << "bytes";

        if (data.size() < 2 || data[1] != char(0x00))
        {
            qCritical() << "[CLIENT] CONNECT failed! REP=" << (quint8)data[1];
            socket->disconnectFromHost();
            return;
        }

        qDebug() << "[CLIENT] CONNECT OK. Tunnel established.";

        clientState = ClientState::Relay;
        break;
    }

    case ClientState::Relay:
        qDebug() << "[CLIENT] [RELAY] Received from server:" << data;
        break;
    }
}

void Client::onDisconnected()
{
    qDebug() << "[CLIENT] Disconnected from proxy.";
    QCoreApplication::quit();
}
