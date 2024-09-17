#ifndef USERSMANAGER_H
#define USERSMANAGER_H

#include <QObject>
#include <QTcpSocket>

#include "tcpsocket.h"
#include "user.h"
#include "room.h"

class UsersManager : public QObject
{
    Q_OBJECT
public:
    UsersManager(QObject * parent = nullptr);

    void connectingSocket(std::shared_ptr<TcpSocket> newSocket);

private slots:
    void readClient();

    void clientDisconnected();
private:

    void signIn(QJsonObject j_obj);
    void logIn(QJsonObject j_obj);

    void handleWrite(QJsonObject obj, TcpSocket * socket_);

    std::vector<std::shared_ptr<TcpSocket>> notAuthSockets;
    std::vector<std::shared_ptr<User >> onlineUsers;
    std::vector<std::shared_ptr<User >> offlineUsers;

    std::vector<std::shared_ptr<Room>> allRooms;
    int idsRoom{0};
};

#endif // USERSMANAGER_H
