#include "user.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

User::User(QString login_, QString password_, QObject *parent) : QObject(parent),
    login(login_),
    password(password_)
{

}

void User::setSocket(std::shared_ptr<TcpSocket> socket)
{
    m_socket = socket;
    connect(m_socket.get(), &QTcpSocket::readyRead, this, &User::readClient);
    connect(m_socket.get(), &QTcpSocket::disconnected, this, &User::clientDisconnected);
    isActive = true;

    QJsonObject obj;
    obj["type"] = "sign";
    obj["status"] = true;
    handleWrite(std::move(obj));

    for (auto & room : rooms){

        QJsonObject obj;
        obj["type"] = "room";
        obj["typeRoom"] = "create";
        obj["nameRoom"] = room.lock()->getName();
        obj["idRoom"] = room.lock()->getIdRoom();
        obj["changeCurrentRoom"] = false;

        QJsonArray message;
        for (auto & mess : room.lock()->getMessage()){
            QJsonObject message_obj;
            message_obj["nameUser"] = mess.nameUser;
            message_obj["mess"] = mess.mess;
            message_obj["typeMessage"] = int(mess.type);

            message.append(message_obj);
        }
        obj["messages"] = message;

        handleWrite(obj);
        room.lock()->changeStatusUser(login, StatusUser::ONLINE);
    }

}

bool User::checkPassword(QString pass)
{
    return pass == password;
}

void User::addRoom(std::weak_ptr<Room> room, bool changeCurrent)
{
    connect(room.lock().get(), &Room::signalNewMessage, [this, room, changeCurrent](Message mes){

        QJsonObject obj;
        obj["type"] = "room";
        obj["typeRoom"] = "message";
        obj["nameRoom"] = room.lock()->getName();;
        obj["nameUser"] = mes.nameUser;
        obj["idRoom"] = room.lock()->getIdRoom();
        obj["message"] = mes.mess;
        obj["typeMessage"] = int(mes.type);
        obj["changeCurrentRoom"] = false;

        handleWrite(obj);
    });
    connect(room.lock().get(), &Room::signalNewUser, [this, room](StatusUser status, QString usrRoom){

        QJsonObject obj;
        obj["type"] = "room";
        obj["typeRoom"] = "addUser";
        obj["nameRoom"] = room.lock()->getName();
        obj["idRoom"] = room.lock()->getIdRoom();
        obj["nameUser"] = usrRoom;
        obj["statusUser"] = int(status);

        handleWrite(obj);
    });
    connect(room.lock().get(), &Room::signalUpdateUser, [this, room](QString usrRoom, StatusUser status){

        QJsonObject obj;
        obj["idRoom"] = room.lock()->getIdRoom();
        obj["nameRoom"] = room.lock()->getName();
        obj["type"] = "room";
        obj["typeRoom"] = "updateUser";
        obj["nameUser"] = usrRoom;
        obj["statusUser"] = int(status);

        handleWrite(obj);
    });
    rooms.emplace_back(room);

    QJsonObject obj;
    obj["type"] = "room";
    obj["typeRoom"] = "create";
    obj["nameRoom"] = room.lock()->getName();
    obj["idRoom"] = room.lock()->getIdRoom();
    obj["changeCurrentRoom"] = changeCurrent;

    QJsonArray message;
    for (auto & mess : room.lock()->getMessage()){
        QJsonObject message_obj;
        message_obj["nameUser"] = mess.nameUser;
        message_obj["mess"] = mess.mess;
        message_obj["typeMessage"] = int(mess.type);

        message.append(message_obj);
    }
    obj["messages"] = message;

    handleWrite(obj);
    room.lock()->addUser(login, isActive ? StatusUser::ONLINE : StatusUser::OFFLINE);
}

void User::readClient()
{
    QByteArray msg = static_cast<TcpSocket *>(sender())->readAll();
    QByteArray msg_2 = msg;
    QByteArray res;

    auto start = msg_2.indexOf("!start");
    while (start != -1){
        auto end = msg_2.indexOf("!end");
        if (end == -1 or start > end) return;

        res = msg_2.mid(start + 6, end-8);
        QJsonParseError error;
        QJsonDocument json = QJsonDocument::fromJson(res, &error);

        if (error.error != QJsonParseError::ParseError::NoError) return;

        QJsonObject j_obj = json.object();

        if (j_obj.contains("type")){

            if (j_obj["type"].toString() == "createRoom"){
                emit signalCreateRoom(j_obj["nameRoom"].toString());
            }else if (j_obj["type"].toString() == "room"){

                if (j_obj["typeRoom"].toString() == "message"){

                    for (auto & room : rooms){
                        if (room.lock()->getIdRoom() == j_obj["idRoom"].toInt()){
                            room.lock()->addMessage(login, j_obj["text"].toString(), TypeMessage(j_obj["typeMessage"].toInt()));
                            break;
                        }
                    }
                }else if(j_obj["typeRoom"].toString() == "changeRoom"){
                    updateRoom(j_obj, true);
                }else if(j_obj["typeRoom"].toString() == "addUser"){
                    signalAddUser(j_obj["idRoom"].toInt(), j_obj["nameUser"].toString());
                }
            }else if (j_obj["type"].toString() == "info"){

                if (j_obj["typeInfo"].toString() == "allUsers"){

                    emit signalRequestAllUsers();
                }
            }
        }

        msg_2.replace(0, end + 4, "");
        start = msg_2.indexOf("!start");
    }
}

void User::clientDisconnected()
{
    isActive = false;
    for (auto & room : rooms){
        room.lock()->changeStatusUser(login, StatusUser::OFFLINE);
    }
    signalDisconnect();
}

void User::handleWrite(QJsonObject obj)
{
    if (isActive){

        QByteArray answer{"!start"};

        answer += QJsonDocument(obj).toJson(QJsonDocument::Compact);

        answer.push_back(0x1);
        answer.push_back(0x1);
        answer.push_back("!end");

        if (m_socket->isOpen()){

            m_socket->write(answer);
        }
    }
}

bool User::checkExistRoom(int id)
{
    return std::all_of(rooms.begin(), rooms.end(), [id](std::weak_ptr<Room> room){return room.lock()->getIdRoom() != id;});
}

void User::updateRoom(QJsonObject obj, bool changeCurrentRoom)
{
    for (auto & room : rooms){
        if (room.lock()->getIdRoom() == obj["idRoom"].toInt()){

            QJsonObject obj;
            obj["type"] = "room";
            obj["typeRoom"] = "openRoom";
            obj["nameRoom"] = room.lock()->getName();
            obj["idRoom"] = room.lock()->getIdRoom();
            obj["changeCurrentRoom"] = changeCurrentRoom;

            QJsonArray message;
            for (auto & mess : room.lock()->getMessage()){
                QJsonObject message_obj;
                message_obj["nameUser"] = mess.nameUser;
                message_obj["mess"] = mess.mess;
                message_obj["typeMessage"] = int(mess.type);

                message.append(message_obj);
            }
            obj["messages"] = message;

            QJsonArray usersRoom;
            for (auto & usr : room.lock()->getUsers()){
                QJsonObject usersRoom_obj;
                usersRoom_obj["nameUser"] = usr.second;
                usersRoom_obj["statusUser"] = int(usr.first);

                usersRoom.append(usersRoom_obj);
            }
            obj["usersRoom"] = usersRoom;
            handleWrite(obj);
            break;
        }
    }
}
