//
// Created by Jiang_Boyuan on 25-10-22.
//

#include "SyncMgr.h"
#include <QJsonDocument>

#include "DataMgr.h"

SyncMgr::SyncMgr(DataMgr* data_mgr, QDateTime last_sync, QSettings* config): data_mgr(data_mgr), last_sync(last_sync), config(config) {
    pwd = config->value("/SN/PWD", "默认密码").toString();
    int hello_port = config->value("/SN/HelloPort", 5051).toInt();

    server_finder = new ServerFinder(pwd, hello_port);
    connect(server_finder, &ServerFinder::server_found, this, &SyncMgr::server_found);
    connect(server_finder, &ServerFinder::server_not_found, this, &SyncMgr::startListening);

    manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &SyncMgr::on_server_reply);

    startListening();
}

SyncMgr::~SyncMgr() {
    delete manager;
    delete server_finder;
}

QString SyncMgr::get_sync_time() {
    return last_sync.toString("yyyy-MM-dd hh:mm:ss");
}
void SyncMgr::set_sync_time(QString time_str) {
    last_sync = QDateTime::fromString(time_str, "yyyy-MM-dd hh:mm:ss");
}

void SyncMgr::server_found(QString server_ip, int server_port) {
    this->server_ip = server_ip;
    this->server_port = server_port;
    startListening();
}

void SyncMgr::on_receive_response(QNetworkReply *reply) {
    while (reply->canReadLine()) {
        QByteArray line = reply->readLine();

        if (line.startsWith("data: ")) {
            QString msg = line.mid(6).trimmed();
            qDebug() << "SyncMgr on_receive_response: " << msg;

            if (last_sync.toString("yyyy-MM-dd hh:mm:ss") != msg) {
                // 需要拉取
                get_data();
            }else if (data_mgr->get_edit_time() > last_sync) {
                // 需要推送
                put_data();
            }
        }
    }
}

void SyncMgr::put_data() {
    QJsonObject doc_data;
    doc_data["tab_list"] = data_mgr->get_tab_list();

    // 将QJsonObject转换为QByteArray
    QJsonDocument doc(doc_data);
    QByteArray jsonData = doc.toJson();

    // 设置请求URL
    QUrl url("http://"+server_ip+":"+QString::number(server_port)+"/api/data/push");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 发送PUT请求
    QNetworkReply *reply = manager->put(request, jsonData);
    connect(reply, &QNetworkReply::readyRead, [this, reply]() {
        on_put_reply(reply);
    });
}

void SyncMgr::get_data() {
    qDebug() << "SyncMgr get_data";
    // 设置请求URL
    QUrl url("http://"+server_ip+":"+QString::number(server_port)+"/api/data/pull");
    QNetworkRequest request(url);
    // 发送GET请求
    QNetworkReply *reply = manager->get(request);
    connect(reply, &QNetworkReply::readyRead, [this, reply]() {
        on_get_reply(reply);
    });
}

void SyncMgr::on_put_reply(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        // 读取响应
        QByteArray response = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
        QJsonObject jsonObject = jsonResponse.object();

        // 处理响应数据
        int result = jsonObject["result"].toInt();
        qDebug() << "Response:" << jsonObject << "result:" << result;

        // 将服务器的数据作为自己的数据
        set_sync_time(jsonObject["sync_time"].toString());
        qDebug() << "new sync time" << last_sync.toString();
    } else {
        // 处理错误
        qDebug() << "Error:" << reply->errorString();
        server_ip = "";
        startListening();
    }
    reply->deleteLater();
}

void SyncMgr::on_get_reply(QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        // 读取响应
        QByteArray response = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
        QJsonObject jsonObject = jsonResponse.object();

        // 处理响应数据
        int result = jsonObject["result"].toInt();
        qDebug() << "Response:" << jsonObject << "result:" << result;

        // 将服务器的数据作为自己的数据
        set_sync_time(jsonObject["sync_time"].toString());
        data_mgr->set_tab_list(jsonObject["tab_list"].toArray());
    } else {
        // 处理错误
        qDebug() << "Error:" << reply->errorString();
        server_ip = "";
        startListening();
    }
    reply->deleteLater();
}

void SyncMgr::on_receive_error(QNetworkReply::NetworkError error) {
    qDebug() << "SyncMgr on_receive_error";
    startListening();
}

void SyncMgr::startListening() {
    qDebug() << "SyncMgr try startListening";
    if (server_ip == "") {
        server_finder->discover_server();
        return;
    }
    QString url = "http://"+server_ip+":"+QString::number(server_port)+"/sync_time";
    qDebug() << "SyncMgr startListening: "<<url;

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "text/event-stream");

    active_reply = manager->get(request);

    connect(active_reply, &QNetworkReply::readyRead, [this]() {
        on_receive_response(active_reply);
    });
    connect(active_reply, &QNetworkReply::errorOccurred, this, &SyncMgr::on_receive_error);
}

void SyncMgr::on_server_reply(QNetworkReply *reply) {
    qDebug() << "SyncMgr on_server_reply";
    if (reply->error() == QNetworkReply::NoError) {
        // 读取响应
        QByteArray response = reply->readAll();
        // 处理响应数据
        qDebug() << "Response:" << response;
    } else {
        // 处理错误
        qDebug() << "Error:" << reply->errorString();
        server_ip = "";
        startListening();
    }

    reply->deleteLater();
}