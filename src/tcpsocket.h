#ifndef TCPSOCKET_H
#define TCPSOCKET_H
#include <QTcpSocket>

class TcpSocket : public QTcpSocket
{
public:
    TcpSocket(QObject *parent = nullptr);

    int getId(){ return id; }
private:
    int id{0};
};

#endif // TCPSOCKET_H
