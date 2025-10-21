//
// Created by Jiang_Boyuan on 25-10-3.
//

#ifndef DATAMGR_H
#define DATAMGR_H

#include <QObject>
#include <QSettings>
#include <QJsonObject> // { }
#include <QJsonArray>
#include <QUdpSocket>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class MainWindow;
class DataMgr :public QObject{
    Q_OBJECT
public:
    DataMgr(QString data_file_path, QSettings* config);
    ~DataMgr();

    QSettings* config;

    // 本地存储相关
    QString data_file_path;
public:
    QJsonArray get_tab_list();
    void load_data();

public slots:
    void update_tab_list(QJsonArray);
    void save_data();

    // 同步相关
public:
    QNetworkAccessManager *manager;
    QUdpSocket *hello_socket;
    QString server_ip;
    int server_port;
    int hello_port;
    QString pwd;
    QDateTime last_sync;
    QDateTime last_edit;
    QJsonArray last_tab_list;
    void discover_server();
public slots:
    void sync_data();
    void server_hello_response();
    void on_server_reply(QNetworkReply *reply);
signals:
    void server_not_found();
    void server_found(QString server_ip, int server_port);
    void sync_complete();
};



#endif //DATAMGR_H
