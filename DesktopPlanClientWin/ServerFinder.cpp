//
// Created by Jiang_Boyuan on 25-10-22.
//

#include "ServerFinder.h"
#include <QNetworkDatagram>
#include <QString>
#include <QTimer>

ServerFinder::ServerFinder(QString pwd, int hello_port): pwd(pwd), hello_port(hello_port){
    hello_socket = new QUdpSocket(this);
    connect(hello_socket, &QUdpSocket::readyRead, this, &ServerFinder::server_hello_response);
}

ServerFinder::~ServerFinder() {
    delete hello_socket;
}

void ServerFinder::discover_server() {
    qDebug() << "ServerFinder discover_server";
    server_ip = "";

    // 发送广播发现消息
    QByteArray datagram = "DISCOVER_FLASK_SERVICE";
    QHostAddress broadcastAddress = QHostAddress("255.255.255.255");

    if (hello_socket->writeDatagram(datagram, broadcastAddress, hello_port) == -1) {
        qDebug() << "broadcast failed:" << hello_socket->errorString();
    } else {
        qDebug() << "broadcast sent";
    }

    // 设置超时
    QTimer::singleShot(3000, this, [this]() {
        if (server_ip == "") {
            qDebug() << "not found server";
            emit server_not_found();
        }
    });
}

void ServerFinder::server_hello_response() {
    qDebug() << "DataMgr discover_server";

    while (hello_socket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = hello_socket->receiveDatagram();
        QString response = QString::fromUtf8(datagram.data());
        QHostAddress sender = datagram.senderAddress();

        qDebug() << "recieve response:" << response << "from:" << sender.toString();

        if (response.startsWith("FLASK_SERVICE:")) {
            QStringList parts = response.split(":");
            if (parts.size() >= 3) {
                server_ip = parts[1];
                server_port = parts[2].toInt();

                qDebug() << "found Flask server:" << server_ip << ":" << server_port;
                emit server_found(server_ip, server_port);
            }
        }
    }
}