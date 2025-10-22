//
// Created by Jiang_Boyuan on 25-10-3.
//

#include "DataMgr.h"

#include <QJsonObject> // { }
#include <QJsonArray> // [ ]
#include <QJsonDocument> // 解析Json
#include <QJsonValue> // int float double bool null { } [ ]
#include <QJsonParseError>
#include <QFile>
#include <QTimer>
#include <QNetworkDatagram>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMessageBox>

DataMgr::DataMgr(QString data_file_path, QSettings* config){
    this->config = config;

    this->data_file_path = data_file_path;

    load_data();
    sync_mgr = new SyncMgr(this, init_sync, config);
}

DataMgr::~DataMgr() {
}

// 本地存储相关
#pragma region 本地存储相关
QDateTime DataMgr::get_edit_time() {
    return last_edit;
}
void DataMgr::set_edit_time(QString time_str) {
    last_edit = QDateTime::fromString(time_str, "yyyy-MM-dd hh:mm:ss");
}

QJsonArray DataMgr::get_tab_list() {
    return last_tab_list;
}
void DataMgr::update_tab_list(QJsonArray new_tab_list) {
    last_edit = QDateTime::currentDateTime();
    last_tab_list = new_tab_list;
    save_data();
}
void DataMgr::set_tab_list(QJsonArray new_tab_list) {
    last_tab_list = new_tab_list;
    save_data();
    emit tab_list_changed();
}

void DataMgr::load_data() {
    qDebug() << "DataMgr load_data";

    // 读取数据文件
    QFile file(data_file_path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << "can't open data file!";
        return;
    }
    QTextStream stream(&file);
    QString str = stream.readAll();
    qDebug() << "str: " << str;
    file.close();

    // 解析json
    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(str.toUtf8(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError && !doc.isNull()) {
        qDebug() << "Json error！" << jsonError.error;
        return ;
    }
    if (!doc.isObject()) {
        qDebug() << "data decode error";
        return ;
    }
    QJsonObject todo_data = doc.object();

    init_sync = QDateTime::fromString(todo_data["sync_time"].toString(), "yyyy-MM-dd hh:mm:ss");
    set_edit_time(todo_data["edit_time"].toString());
    last_tab_list = todo_data["tab_list"].toArray();
}

void DataMgr::save_data() {
    qDebug() << "DataMgr save_data";

    // 处理json
    QJsonObject todo_data;
    todo_data.insert("sync_time", sync_mgr->get_sync_time());
    todo_data.insert("edit_time", get_edit_time().toString("yyyy-MM-dd hh:mm:ss"));
    todo_data.insert("tab_list", last_tab_list);

    QJsonDocument doc;
    doc.setObject(todo_data);

    // 写入文件
    QFile file(data_file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << "can't open error!";
        return;
    }
    QTextStream stream(&file);
    stream << doc.toJson();
    file.close();
}
#pragma endregion 本地存储相关
