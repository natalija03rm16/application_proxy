#ifndef AUTHENTICATE_H
#define AUTHENTICATE_H

#include <QString>

class Authenticate
{
public:
    Authenticate();

    bool login(const QString& username, const QString& password);

private:
    QString hashPassword(const QString& password);
};

#endif
