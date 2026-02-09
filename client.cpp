#include "client.h"
#include <QInputDialog>
#include <QLineEdit>
#include <QDebug>

Client::Client(const QString& proxyHost, quint16 proxyPort, QObject* parent)
    : QObject(parent)
{
    QString username = QInputDialog::getText(nullptr, "Login", "Username:");
    QString password = QInputDialog::getText(nullptr, "Login", "Password:", QLineEdit::Password);

    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &Client::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);
    connect(socket, &QTcpSocket::disconnected, this, &Client::onDisconnected);

    socket->connectToHost(proxyHost, proxyPort);

    // Poslati username i password nakon konekcije
    connect(socket, &QTcpSocket::connected, [=]() {
        QByteArray auth;
        auth.append(username.toUtf8());
        auth.append(':');
        auth.append(password.toUtf8());
        socket->write(auth);
    });
}

void Client::onConnected() {
    qDebug() << "Connected to proxy!";
}

void Client::sendMessage(const QString& message) {
    if (socket->state() == QTcpSocket::ConnectedState)
        socket->write(message.toUtf8());
}

void Client::onReadyRead() {
    QByteArray data = socket->readAll();
    qDebug() << "Client received:" << data;
}

void Client::onDisconnected() {
    qDebug() << "Disconnected from proxy.";
}
