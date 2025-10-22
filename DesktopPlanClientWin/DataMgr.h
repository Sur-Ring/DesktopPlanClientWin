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

#include "SyncMgr.h"

class MainWindow;
class DataMgr :public QObject{
    Q_OBJECT
public:
    DataMgr(QString data_file_path, QSettings* config);
    ~DataMgr();

    QSettings* config;
    SyncMgr* sync_mgr;

    // 本地存储相关
    QString data_file_path;

    QDateTime get_edit_time();
    void set_edit_time(QString);
    QJsonArray get_tab_list();
    void load_data();
private:
    QDateTime init_sync;
    QDateTime last_edit;
    QJsonArray last_tab_list;
public slots:
    void update_tab_list(QJsonArray);
    void set_tab_list(QJsonArray);
    void save_data();
signals:
    void tab_list_changed();

};



#endif //DATAMGR_H
