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

    pwd = config->value("/SN/PWD", "默认密码").toString();
    hello_socket = new QUdpSocket(this);
    hello_port = config->value("/SN/HelloPort", 5051).toInt();
    connect(hello_socket, &QUdpSocket::readyRead, this, &DataMgr::server_hello_response);
    discover_server();
    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &DataMgr::on_server_reply);

    load_data();
    if (last_sync != last_edit) {
        sync_data();
    }
}

DataMgr::~DataMgr() {
}

// 本地存储相关
#pragma region 本地存储相关
QJsonArray DataMgr::get_tab_list() {
    return last_tab_list;
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

    last_sync = QDateTime::fromString(todo_data["sync_time"].toString(), "yyyy-MM-dd hh:mm:ss");
    last_edit = QDateTime::fromString(todo_data["edit_time"].toString(), "yyyy-MM-dd hh:mm:ss");
    last_tab_list = todo_data["tab_list"].toArray();
}

void DataMgr::update_tab_list(QJsonArray new_tab_list) {
    last_tab_list = new_tab_list;
    last_edit = QDateTime::currentDateTime();
    save_data();
}

void DataMgr::save_data() {
    qDebug() << "DataMgr save_data";

    // 处理json
    QJsonObject todo_data;
    todo_data.insert("sync_time", last_sync.toString("yyyy-MM-dd hh:mm:ss"));
    todo_data.insert("edit_time", last_edit.toString("yyyy-MM-dd hh:mm:ss"));
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

    // 同步
    if (last_sync != last_edit) {
        sync_data();
    }
}
#pragma endregion 本地存储相关

// 同步相关
#pragma region 同步相关
void DataMgr::sync_data() {
    qDebug() << "DataMgr sync_data";
    if (server_ip == "") {
        discover_server();
        return;
    }

    QJsonObject doc_data;

    qDebug() << "sync_time send:" <<  last_sync.toString();
    doc_data["sync_time"] = last_sync.toString("yyyy-MM-dd hh:mm:ss");
    doc_data["tab_list"] = last_tab_list;

    // 将QJsonObject转换为QByteArray
    QJsonDocument doc(doc_data);
    QByteArray jsonData = doc.toJson();

    // 设置请求URL
    QUrl url("http://"+server_ip+":"+QString::number(server_port)+"/api/data/push");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 发送POST请求
    manager->post(request, jsonData);
}

void DataMgr::on_server_reply(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        // 读取响应
        QByteArray response = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
        QJsonObject jsonObject = jsonResponse.object();

        // 处理响应数据
        int result = jsonObject["result"].toInt();
        qDebug() << "Response:" << jsonObject << "result:" << result;
        if (result == -1) {
            qDebug() << "sync fail";
            // 如果同步失败, 那么直接将服务器的数据作为自己的数据
            last_sync = QDateTime::fromString(jsonObject["sync_time"].toString(),"yyyy-MM-dd hh:mm:ss");
            last_edit = last_sync;
            last_tab_list = jsonObject["tab_list"].toArray();
            qDebug() << "new sync time" << last_sync.toString();
            save_data();
            emit sync_complete();
            // 应该接下来再推送一次细分的更改, 如果后面用数据库的话
        }
        else if (result == 0) {
            qDebug() << "sync success";
            // 同步成功, 更新同步时间
            last_sync = QDateTime::fromString(jsonObject["sync_time"].toString(),"yyyy-MM-dd hh:mm:ss");
            last_edit = last_sync;
            save_data();
        }
    } else {
        // 处理错误
        qDebug() << "Error:" << reply->errorString();
        server_ip = "";
    }

    reply->deleteLater();
}

void DataMgr::discover_server() {
    qDebug() << "DataMgr discover";
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

void DataMgr::server_hello_response() {
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
#pragma endregion 同步相关
