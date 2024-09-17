#include "usersmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonArray>
#include <iostream>
#include <memory>

UsersManager::UsersManager(QObject *parent) : QObject(parent)
{

}

void UsersManager::connectingSocket(std::shared_ptr<TcpSocket> newSocket)
{
    connect(newSocket.get(), &QTcpSocket::readyRead, this, &UsersManager::readClient);
    connect(newSocket.get(), &QTcpSocket::disconnected, this, &UsersManager::clientDisconnected);
    notAuthSockets.emplace_back(newSocket);
}

void UsersManager::readClient()
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

        ///ТЕЛО
        if (j_obj.contains("type")){

            std::cout << "Получено сообщение на вход/регистрацию" << std::endl;
            if (j_obj["type"].toString() == "logIn"){ //Регистраци

                logIn(j_obj);
            }else if (j_obj["type"].toString() == "signIn"){ //Вход

                signIn(j_obj);
            }
        }

        msg_2.replace(0, end + 4, "");
        start = msg_2.indexOf("!start");
    }


}

void UsersManager::clientDisconnected()
{
    auto socket = static_cast<TcpSocket *>(sender());
    auto index{0};
    for (auto &s : notAuthSockets){
        if (s->getId() == socket->getId()){
            notAuthSockets.erase(notAuthSockets.begin() + index);
            break;
        }
        ++index;
    }
}

void UsersManager::signIn(QJsonObject j_obj)
{
    if (std::any_of(offlineUsers.cbegin(), offlineUsers.cend(),
                    [j_obj](std::shared_ptr<User> user_) { return user_->getLogin() == j_obj["login"].toString(); })){

        ///Вход
        auto id_socket = static_cast<TcpSocket *>(sender())->getId();
        for (size_t offUser{0}; offUser < offlineUsers.size(); ++offUser){

            if (offlineUsers.at(offUser)->getLogin() == j_obj["login"].toString() and offlineUsers.at(offUser)->checkPassword(j_obj["password"].toString())){

                offlineUsers.at(offUser)->disconnect();
                for (size_t i{0}; i < notAuthSockets.size(); ++i){
                    if (notAuthSockets.at(i)->getId()== id_socket){
                        notAuthSockets.at(i)->disconnect();
                        offlineUsers.at(offUser)->setSocket(notAuthSockets.at(i));
                        notAuthSockets.erase(notAuthSockets.begin()+int(i));
                        break;
                    }
                }
                connect(offlineUsers.at(offUser).get(), &User::signalRequestAllUsers, [this, user = offlineUsers.at(offUser)]{
                    QJsonObject obj;
                    obj["type"] = "info";
                    obj["typeInfo"] = "allUsers";
                    QJsonArray objUsers;
                    for(auto & usr : offlineUsers){
                        if (user->getLogin() != usr->getLogin()){

                            QJsonObject j_users;
                            j_users["name"] = usr->getLogin();
                            objUsers.append(j_users);
                        }
                    }
                    for(auto & usr : onlineUsers){
                        if (user->getLogin() != usr->getLogin()){

                            QJsonObject j_users;
                            j_users["name"] = usr->getLogin();
                            objUsers.append(j_users);
                        }
                    }
                    obj["users"] = objUsers;

                    user->handleWrite(obj);
                });
                connect(offlineUsers.at(offUser).get(), &User::signalCreateRoom, [this, user = offlineUsers.at(offUser)](QString name){

                    auto room = new Room(name, ++idsRoom, this);
                    allRooms.emplace_back(room);
                    user->addRoom(allRooms.at(allRooms.size() - 1), true);
                });
                connect(offlineUsers.at(offUser).get(), &User::signalDisconnect, [this, user = offlineUsers.at(offUser)]{

                    for (size_t index{0}; index < onlineUsers.size(); ++index){
                        if (onlineUsers.at(index)->getLogin() == user->getLogin()){
                            user->disconnect();
                            offlineUsers.push_back(user);
                            onlineUsers.erase(onlineUsers.begin() + int(index));
                            break;
                        }
                    }
                });
                connect(offlineUsers.at(offUser).get(), &User::signalAddUser, [this, user = offlineUsers.at(offUser).get()](int id, QString addingUser){

                    for (auto & room : allRooms){
                        if (room->getIdRoom() == id){
                            bool exist{false};
                            for (auto & usr : onlineUsers){

                                if (usr->getLogin() == addingUser and usr->checkExistRoom(id)){
                                    usr->addRoom(room, false);
                                    exist = true;
                                    break;
                                }
                            }

                            for (auto & usr : offlineUsers){
                                if (usr->getLogin() == addingUser and usr->checkExistRoom(id)){
                                    usr->addRoom(room, false);
                                    exist = true;
                                    break;
                                }
                            }

                            QJsonObject obj;
                            obj["type"] = "info";
                            obj["typeInfo"] = "addUser";
                            if (exist){
                                obj["status"] = true;
                                obj["message"] = "Пользователь добавлен";
                            }else{
                                obj["status"] = false;
                                obj["message"] = "Такого пользователя не существует";
                            }
                            user->handleWrite(obj);
                        }
                    }
                });
                onlineUsers.push_back(offlineUsers.at(offUser));
                offlineUsers.erase(offlineUsers.begin() + int(offUser));
                std::cout << "Пользователь вошел" << std::endl;
                break;
            }else{
                ///Вернуть ошибку входа (неверный пароль)
                QJsonObject obj;
                obj["type"] = "sign";
                obj["status"] = false;
                obj["message"] = "Неправильный пароль";
                handleWrite(std::move(obj), static_cast<TcpSocket *>(sender()));
            }
        }
    }else{
        ///Вернуть ошибку входа (нет такого пользователя)
        QJsonObject obj;
        obj["type"] = "sign";
        obj["status"] = false;
        obj["message"] = "Нет такого пользователя, или вход уже выполнен";
        handleWrite(std::move(obj), static_cast<TcpSocket *>(sender()));
    }
}

void UsersManager::logIn(QJsonObject j_obj)
{
    if (std::none_of(offlineUsers.cbegin(), offlineUsers.cend(),
                    [j_obj](std::shared_ptr<User> user_) { return user_->getLogin() == j_obj["login"].toString(); })
            and std::none_of(onlineUsers.cbegin(), onlineUsers.cend(),
                             [j_obj](std::shared_ptr<User> user_) { return user_->getLogin() == j_obj["login"].toString(); })){


        ///Регистрация
        auto user = new User(j_obj["login"].toString(), j_obj["password"].toString());
        auto id_socket = static_cast<TcpSocket *>(sender())->getId();
        for (size_t i{0}; i < notAuthSockets.size(); ++i){
            if (notAuthSockets.at(i)->getId()== id_socket){
                notAuthSockets.at(i)->disconnect();
                user->setSocket(notAuthSockets.at(i));
                notAuthSockets.erase(notAuthSockets.begin()+int(i));
                break;
            }
        }
        auto creatingUser = onlineUsers.emplace_back(user);
        connect(creatingUser.get(), &User::signalRequestAllUsers, [this, user = creatingUser]{
            QJsonObject obj;
            obj["type"] = "info";
            obj["typeInfo"] = "allUsers";
            QJsonArray objUsers;
            for(auto & usr : offlineUsers){
                if (user->getLogin() != usr->getLogin()){

                    QJsonObject j_users;
                    j_users["name"] = usr->getLogin();
                    objUsers.append(j_users);
                }
            }
            for(auto & usr : onlineUsers){
                if (user->getLogin() != usr->getLogin()){

                    QJsonObject j_users;
                    j_users["name"] = usr->getLogin();
                    objUsers.append(j_users);
                }
            }
            obj["users"] = objUsers;

            user->handleWrite(obj);
        });
        connect(creatingUser.get(), &User::signalCreateRoom, [this, creatingUser](QString name){

            auto room = new Room(name, ++idsRoom, this);
            allRooms.emplace_back(room);
            creatingUser->addRoom(allRooms.at(allRooms.size() - 1), true);
        });
        connect(creatingUser.get(), &User::signalDisconnect, [this, creatingUser]{

            for (size_t index{0}; index < onlineUsers.size(); ++index){
                if (onlineUsers.at(index)->getLogin() == creatingUser->getLogin()){
                    creatingUser->disconnect();
                    offlineUsers.push_back(creatingUser);
                    onlineUsers.erase(onlineUsers.begin() + int(index));
                    break;
                }
            }
        });
        connect(creatingUser.get(), &User::signalAddUser, [this, creatingUser](int id, QString addingUser){

            for (auto & room : allRooms){
                if (room->getIdRoom() == id){
                    bool exist{false};
                    for (auto & usr : onlineUsers){

                        if (usr->getLogin() == addingUser and usr->checkExistRoom(id)){
                            usr->addRoom(room, false);
                            exist = true;
                            break;
                        }
                    }

                    for (auto & usr : offlineUsers){
                        if (usr->getLogin() == addingUser and usr->checkExistRoom(id)){
                            usr->addRoom(room, false);
                            exist = true;
                            break;
                        }
                    }

                    QJsonObject obj;
                    obj["type"] = "info";
                    obj["typeInfo"] = "addUser";
                    if (exist){
                        obj["status"] = true;
                        obj["message"] = "Пользователь добавлен";
                    }else{
                        obj["status"] = false;
                        obj["message"] = "Такого пользователя не существует";
                    }
                    creatingUser->handleWrite(obj);
                }
            }
        });
        std::cout << "Пользователь зарегистрирован" << std::endl;
    }else{
        ///Вернуть ошибку создания пользователя ()
        QJsonObject obj;
        obj["type"] = "sign";
        obj["status"] = false;
        obj["message"] = "Пользователь с таким логином уже существует";
        handleWrite(std::move(obj), static_cast<TcpSocket *>(sender()));
    }
}

void UsersManager::handleWrite(QJsonObject obj, TcpSocket *socket_)
{
    if (socket_->isOpen()){

        QByteArray answer{"!start"};

        answer += QJsonDocument(obj).toJson(QJsonDocument::Compact);

        answer.push_back(0x1);
        answer.push_back(0x1);
        answer.push_back("!end");


        socket_->write(answer);
    }
}
