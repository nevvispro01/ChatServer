#ifndef USER_H
#define USER_H
#include <QObject>
#include <memory>
#include "tcpsocket.h"
#include "room.h"

class User : public QObject
{
    Q_OBJECT
public:
    User(QString login_, QString password_, QObject * parent = nullptr);

    void setSocket(std::shared_ptr<TcpSocket> socket);

    QString & getLogin(){ return login; }

    bool checkPassword(QString pass);

    void addRoom(std::weak_ptr<Room> room, bool changeCurrent);

    void handleWrite(QJsonObject obj);

    bool checkExistRoom(int id);

signals:

    void signalCreateRoom(QString name);
    void signalAddUser(int id, QString addingUser);
    void signalDisconnect();

    void signalRequestAllUsers();

private slots:
    void readClient();

    void clientDisconnected();

private:

    void updateRoom(QJsonObject obj, bool changeCurrentRoom);

    bool isActive{false};
    QString login;
    QString password;
    std::shared_ptr<TcpSocket> m_socket;
    std::vector<std::weak_ptr<Room>> rooms;
};

#endif // USER_H
