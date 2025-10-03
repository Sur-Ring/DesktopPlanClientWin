//
// Created by Jiang_Boyuan on 25-10-3.
//

#ifndef DATAMGR_H
#define DATAMGR_H

#include <QObject>
#include <QSettings>
#include <QJsonObject> // { }
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
    QJsonObject load_data();
public slots:
    void save_data(QJsonObject todo_data);

    // 同步相关
public:
    QNetworkAccessManager *manager;
    bool need_sync;
    QUdpSocket *hello_socket;
    QString server_ip;
    int server_port;
    int hello_port;
    QString pwd;
    QDateTime last_sync;
    QDateTime last_edit;
    void discover_server();
public slots:
    void sync_data(QJsonObject todo_data);
    void server_hello_response();
    void on_server_reply(QNetworkReply *reply);
signals:
    void server_not_found();
    void server_found(QString server_ip, int server_port);
};



#endif //DATAMGR_H
