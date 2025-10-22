//
// Created by Jiang_Boyuan on 25-10-22.
//

#ifndef SERVERFINDER_H
#define SERVERFINDER_H

#include <QObject>
#include <QSettings>
#include <QUdpSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class ServerFinder :public QObject{
    Q_OBJECT
public:
    ServerFinder(QString pwd, int hello_port);
    ~ServerFinder();

    QUdpSocket *hello_socket;

    int hello_port;
    QString pwd;

    QString server_ip;
    int server_port;

    void discover_server();
public slots:
    void server_hello_response();
signals:
    void server_not_found();
    void server_found(QString server_ip, int server_port);
};



#endif //SERVERFINDER_H
