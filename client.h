#ifndef CLIENT_H
#define CLIENT_H

#include <QtNetwork/QTcpSocket>
#include <QObject>

class Client : public QObject
{
    Q_OBJECT
public:
    Client(const QString& proxyHost, quint16 proxyPort, QObject* parent = nullptr);
    void sendMessage(const QString& message);

private slots:
    void onConnected();
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket* socket;
};

#endif // CLIENT_H
