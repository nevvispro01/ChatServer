#include "room.h"

Room::Room(QString & name, int id_, QObject *parent) : QObject(parent),
    roomName(name),
    id(id_)
{

}

QString Room::getName()
{
    return roomName;
}

std::vector<Message> & Room::getMessage()
{
    return allMessage;
}

std::vector<std::pair<StatusUser, QString>> &Room::getUsers()
{
    return users;
}

int &Room::getIdRoom()
{
    return id;
}

void Room::addUser(QString &login, StatusUser status)
{
    users.emplace_back(std::pair(status, login));
    emit signalNewUser(status, login);
}

void Room::changeStatusUser(QString &login, StatusUser status)
{
    for (auto & usr : users){
        if (usr.second == login){
            usr.first = status;
        }
    }
    emit signalUpdateUser(login, status);
}

void Room::addMessage(QString nameUser_, QString mess_, TypeMessage typeMess)
{
    Message mes;
    mes.nameUser = nameUser_;
    mes.mess = mess_;
    mes.type = typeMess;
    allMessage.emplace_back(mes);
    emit signalNewMessage(mes);
}
