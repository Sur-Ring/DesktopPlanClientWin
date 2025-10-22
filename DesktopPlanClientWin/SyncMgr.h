//
// Created by Jiang_Boyuan on 25-10-22.
//

#ifndef SYNCMGR_H
#define SYNCMGR_H

#include <QObject>
#include <QSettings>
#include <QJsonObject> // { }
#include <QJsonArray>
#include <QUdpSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "ServerFinder.h"

class DataMgr;
class SyncMgr :public QObject{
    Q_OBJECT
public:
    SyncMgr(DataMgr* data_mgr, QDateTime last_sync, QSettings* config);
    ~SyncMgr();
    DataMgr* data_mgr;
    QSettings* config;

    ServerFinder* server_finder;
    QNetworkAccessManager *manager;
    QString server_ip;
    int server_port;
    QString pwd;

    QNetworkReply* active_reply;
    void startListening();
    QString get_sync_time();
    void set_sync_time(QString);
private:
    QDateTime last_sync;
public slots:
    void put_data();
    void get_data();
    void on_put_reply(QNetworkReply *reply);
    void on_get_reply(QNetworkReply *reply);
    void on_receive_response(QNetworkReply *reply);
    void on_receive_error(QNetworkReply::NetworkError);
    void server_found(QString server_ip, int server_port);
    void on_server_reply(QNetworkReply *reply);
signals:
    void sync_complete();
};



#endif //SYNCMGR_H
