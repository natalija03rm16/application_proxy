#include <QApplication>
#include <QTimer>
#include <QDebug>
#include "server.h"
#include "proxy.h"
#include "client.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QStringList args;

    if (argc > 1) {
        args = QStringList(argv + 1, argv + argc);
    } else {
        qCritical() << "Usage:";
        qCritical() << "  ./application_proxy server";
        qCritical() << "  ./application_proxy proxy";
        qCritical() << "  ./application_proxy client";
        return -1;
    }

    QString mode = args.at(0);

    if (mode == "server") {
        qDebug() << "Starting SERVER...";
        Server* server = new Server(12345, &a);
        Q_UNUSED(server);
    }
    else if (mode == "proxy") {
        qDebug() << "Starting PROXY...";
        Proxy* proxy = new Proxy(54321, &a);
        Q_UNUSED(proxy);
    }
    else if (mode == "client") {
        qDebug() << "Starting CLIENT...";
        Client* client = new Client("127.0.0.1", 54321, &a);

        QTimer::singleShot(1000, [client]() {
            client->sendMessage("Hello through proxy!");
        });

        QTimer::singleShot(2000, [client]() {
            client->askAndSendFile();
        });

        Q_UNUSED(client);
    }
    else {
        qCritical() << "Unknown mode:" << mode;
        return -1;
    }

    return a.exec();
}
