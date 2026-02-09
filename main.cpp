#include <QApplication>
#include <QTimer>
#include <QDebug>
#include "server.h"
#include "proxy.h"
#include "client.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QStringList args = a.arguments();

    if (args.size() < 2) {
        qCritical() << "Usage:";
        qCritical() << "  ./application_proxy server";
        qCritical() << "  ./application_proxy proxy";
        qCritical() << "  ./application_proxy client";
        return -1;
    }

    QString mode = args.at(1);

    if (mode == "server") {
        qDebug() << "Starting SERVER...";
        Server server(12345);
        return a.exec();
    }

    if (mode == "proxy") {
        qDebug() << "Starting PROXY...";
        Proxy proxy(54321, "127.0.0.1", 12345);
        return a.exec();
    }

    if (mode == "client") {
        qDebug() << "Starting CLIENT...";
        Client client("127.0.0.1", 54321);

        QTimer::singleShot(1000, [&client]() {
            client.sendMessage("Hello through proxy!");
        });

        return a.exec();
    }

    qCritical() << "Unknown mode:" << mode;
    return -1;
}
