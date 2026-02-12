#include "authenticate.h"
#include <QFile>
#include <QTextStream>

Authenticate::Authenticate() {}

QString Authenticate::hashPassword(const QString& password)
{
    quint32 hash = 0;
    for (QChar c : password)
    {
        hash = hash * 31 + c.unicode();
    }
    return QString::number(hash);
}

bool Authenticate::login(const QString& username, const QString& password)
{
    QFile file("/home/natalija/Desktop/mrkirm/application_proxy/login_info/pairs.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);
    QString hashedInput = hashPassword(password);

    while (!in.atEnd())
    {
        QString line = in.readLine();
        QStringList parts = line.split(':');
        if (parts.size() == 2)
        {
            QString fileUsername = parts[0];
            QString filePassword = parts[1];

            if (fileUsername == username && filePassword == hashedInput)
                return true;
        }
    }

    return false;
}
