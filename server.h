#ifndef SERVER_H
#define SERVER_H

#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QObject>

class Server : public QObject
{
    Q_OBJECT
public:
    Server(quint16 port, QObject* parent = nullptr);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    QTcpServer* server;
    QTcpSocket* clientSocket;
};

#endif
