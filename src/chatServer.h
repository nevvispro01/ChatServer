#ifndef CHATSERVER_H
#define CHATSERVER_H

#include <QtNetwork>
#include "usersmanager.h"

class ChatServer : public QTcpServer {
    Q_OBJECT

public:
    ChatServer(QObject *parent = nullptr) : QTcpServer(parent) {}

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    std::unique_ptr<UsersManager> userManager{new UsersManager};
    std::shared_ptr<TcpSocket> socket;
    int id{0};
};
#endif // CHATSERVER_H
