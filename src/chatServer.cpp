#include "chatServer.h"

void ChatServer::incomingConnection(qintptr socketDescriptor) {
    socket.reset(new TcpSocket());

    if (socket->setSocketDescriptor(socketDescriptor)) {
        userManager->connectingSocket(socket);
    } else {
    }
}
