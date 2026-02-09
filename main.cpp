#include <QTimer>
#include <QDebug>
#include <QApplication>

#include "server.h"
#include "proxy.h"
#include "client.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QStringList args = a.arguments();

    if (args.size() < 2) {
        qCritical() << "Usage:";
        qCritical() << "  socks_app server";
        qCritical() << "  socks_app proxy";
        qCritical() << "  socks_app client";
        return -1;
    }

    QString mode = args.at(1);

    // ================= SERVER =================
    if (mode == "server") {
        qDebug() << "Starting SERVER...";
        Server server(12345);   // server sluša na 12345
        return a.exec();
    }

    // ================= PROXY =================
    if (mode == "proxy") {
        qDebug() << "Starting PROXY...";
        Proxy proxy(
            54321,          // proxy sluša ovde
            "127.0.0.1",    // server IP
            12345           // server port
            );
        return a.exec();
    }

    // ================= CLIENT =================
    if (mode == "client") {
        qDebug() << "Starting CLIENT...";
        Client client("127.0.0.1", 54321);

        // pošalji poruku posle 1s
        QTimer::singleShot(1000, [&client]() {
            client.sendMessage("Hello through proxy!");
        });

        return a.exec();
    }

    qCritical() << "Unknown mode:" << mode;
    return -1;
}
