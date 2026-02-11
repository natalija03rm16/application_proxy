#ifndef AUTHENTICATE_H
#define AUTHENTICATE_H

#include <QString>

class Authenticate
{
public:
    Authenticate(const QString& filePath);

    bool login(const QString& username, const QString& password);

private:
    QString m_filePath;

    QString hashPassword(const QString& password);
};

#endif
