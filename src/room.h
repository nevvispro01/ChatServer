#ifndef ROOM_H
#define ROOM_H
#include <QObject>
#include <memory>

enum class TypeMessage{
    MESSAGE,
    SMILE
};

enum class StatusUser{
    ONLINE,
    OFFLINE
};

struct Message{
    QString nameUser;
    QString mess;
    TypeMessage type;
};

class Room : public QObject
{
    Q_OBJECT
public:
    Room(QString & name, int id_, QObject * parent = nullptr);

    QString getName();
    std::vector<Message> & getMessage();
    std::vector<std::pair<StatusUser, QString>> & getUsers();
    int & getIdRoom();
    void addUser(QString &login, StatusUser status);

    void changeStatusUser(QString &login, StatusUser status);

    void addMessage(QString nameUser_, QString mess_, TypeMessage typeMess);

signals:
    void signalNewMessage(Message & mess);
    void signalNewUser(StatusUser status, QString userRoom);
    void signalUpdateUser(QString &login, StatusUser status);

private:

    QString roomName;
    int id;
    std::vector<Message> allMessage;
    std::vector<std::pair<StatusUser, QString>> users;
};

#endif // ROOM_H
