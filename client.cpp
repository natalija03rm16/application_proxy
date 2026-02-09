#include "client.h"
#include <QDebug>
#include <QInputDialog>
#include <QLineEdit>
#include <QCoreApplication>

Client::Client(const QString& proxyHost, quint16 proxyPort, QObject* parent)
    : QObject(parent)
{
    // GUI unos username/password
    username = QInputDialog::getText(nullptr, "Login", "Username:");
    password = QInputDialog::getText(nullptr, "Login", "Password:", QLineEdit::Password);

    socket = new QTcpSocket(this);
    connect(socket, &QTcpSocket::connected, this, &Client::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &Client::onReadyRead);

    socket->connectToHost(proxyHost, proxyPort);
}

void Client::onConnected()
{
    qDebug() << "Connected to proxy!";

    // ---------------- SOCKS v5 HELLO ----------------
    QByteArray hello;
    hello.append(char(0x05)); // VER
    hello.append(char(0x01)); // NMETHODS
    hello.append(char(0x02)); // METHOD = username/password
    socket->write(hello);
    socket->flush();
}

void Client::onReadyRead()
{
    QByteArray data = socket->readAll();

    if (!authStep1Done) {
        // Proxy odgovara na HELLO
        if (data.size() >= 2 && static_cast<unsigned char>(data[0]) == 0x05) {
            if (static_cast<unsigned char>(data[1]) == 0x02) {
                // Proxy traži username/password
                QByteArray auth;
                auth.append(char(0x01)); // VER
                auth.append(char(username.size()));
                auth.append(username.toUtf8());
                auth.append(char(password.size()));
                auth.append(password.toUtf8());
                socket->write(auth);
                socket->flush();
                authStep1Done = true;
                return;
            }
        }
    }

    if (!authStep2Done) {
        // Proxy odgovara na username/password
        if (data.size() >= 2 && static_cast<unsigned char>(data[0]) == 0x01) {
            if (static_cast<unsigned char>(data[1]) == 0x00) {
                // Autentifikacija uspešna
                authStep2Done = true;
                qDebug() << "Authentication successful!";

                // Nakon uspešne autentifikacije, šaljemo test poruku
                sendMessage("Hello through proxy!");
            } else {
                qDebug() << "Authentication failed!";
                socket->disconnectFromHost();
                QCoreApplication::quit(); // izlazimo iz programa
            }
            return;
        }
    }

    // Kada je autentifikacija prošla, primamo podatke od servera
    if (authStep2Done) {
        qDebug() << "Client received:" << data;
    }
}

void Client::sendMessage(const QString& message)
{
    if (socket->state() == QTcpSocket::ConnectedState)
        socket->write(message.toUtf8());
}
