#include <QCoreApplication>
#include <iostream>
#include "src/chatServer.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    ChatServer server;
        if (!server.listen(QHostAddress::Any, 11001)) {
            return 1;
        }

    return a.exec();
}
