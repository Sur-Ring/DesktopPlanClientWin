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
    last_sync = config->value("/SN/LastSync", "2025-10-01 00:00:00").toDateTime();
    last_edit = config->value("/SN/LastEdit", "2025-10-01 00:00:00").toDateTime();
    hello_socket = new QUdpSocket(this);
    hello_port = config->value("/SN/HelloPort", 5051).toInt();
    connect(hello_socket, &QUdpSocket::readyRead, this, &DataMgr::server_hello_response);
    if (last_edit>last_sync) {
        need_sync = true;
    }else {
        need_sync = false;
    }
    discover_server();
    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &DataMgr::on_server_reply);
}

DataMgr::~DataMgr() {
    config->setValue("/SN/LastSync", last_sync);
    config->setValue("/SN/LastEdit", last_edit);
}

// 本地存储相关
#pragma region 本地存储相关
QJsonObject DataMgr::load_data() {
    qDebug() << "DataMgr load_data";

    // 读取数据文件
    QFile file(data_file_path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << "can't open data file!";
        return {};
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
        return {};
    }
    if (!doc.isObject()) {
        qDebug() << "data decode error";
        return {};
    }

    return doc.object();
}

void DataMgr::save_data(QJsonObject todo_data) {
    qDebug() << "DataMgr save_data";

    QJsonDocument doc;
    doc.setObject(todo_data);

    QFile file(data_file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << "can't open error!";
        return;
    }
    QTextStream stream(&file);
    stream << doc.toJson();
    file.close();

    last_edit = QDateTime::currentDateTime();
    sync_data(todo_data);
}
#pragma endregion 本地存储相关

// 同步相关
#pragma region 同步相关
void DataMgr::sync_data(QJsonObject todo_data) {
    qDebug() << "DataMgr sync_data";
    if (server_ip == "") {
        discover_server();
        return;
    }

    QJsonObject doc_data;

    doc_data["sync_time"] = last_sync.toString("yyyy-MM-dd hh:mm:ss");
    doc_data["edit_time"] = last_edit.toString("yyyy-MM-dd hh:mm:ss");
    doc_data["todo_data"] = todo_data;

    // 将QJsonObject转换为QByteArray
    QJsonDocument doc(doc_data);
    QByteArray jsonData = doc.toJson();

    // 设置请求URL
    QUrl url("http://"+server_ip+":"+QString::number(server_port)+"/api/data/check");
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
        int sync_stat = jsonObject["sync_stat"].toInt();
        qDebug() << "Response:" << jsonObject << "stat:" << sync_stat;
        if (sync_stat == 1) {
            last_sync = QDateTime::fromString(jsonObject["sync_time"].toString(),"yyyy-MM-dd hh:mm:ss");
        }
        else if (sync_stat == 2) {
            last_sync = QDateTime::fromString(jsonObject["sync_time"].toString(),"yyyy-MM-dd hh:mm:ss");
            save_data(jsonObject["server_data"].toObject());
        }
        else if (sync_stat == 3) {
            QMessageBox::StandardButton choose;
            choose = QMessageBox::question(NULL, "修改冲突", "以服务端为准？", QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
            if (choose == QMessageBox::Yes) {
                // 用户点击了Yes
                save_data(jsonObject["server_data"].toObject());
                last_sync = QDateTime::fromString(jsonObject["sync_time"].toString(),"yyyy-MM-dd hh:mm:ss");
            } else {
                // 用户点击了No
                last_sync = QDateTime::fromString(jsonObject["sync_time"].toString(),"yyyy-MM-dd hh:mm:ss");
            }
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
