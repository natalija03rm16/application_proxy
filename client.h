#ifndef CLIENT_H
#define CLIENT_H
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>
#include <QObject>

enum class ClientState {
    Greeting,
    Auth,
    Request,
    Relay
};

class Client : public QObject
{
    Q_OBJECT
public:
    Client(const QString& proxyHost, quint16 proxyPort, QObject* parent = nullptr);
    ~Client();
    void sendMessage(const QString& message);

private slots:
    void onConnected();
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket* socket;
    ClientState clientState;
};

#endif // CLIENT_H
