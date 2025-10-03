//
// Created by Jiang_Boyuan on 25-10-3.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Todo_Tab.h" resolved

#include "todo_tab.h"
#include "ui_Todo_Tab.h"

#include <QJsonObject> // { }
#include <QJsonArray> // [ ]
#include <QJsonDocument> // 解析Json
#include <QJsonValue> // int float double bool null { } [ ]

#include "todo_entry.h"
#include "DDL_Parser.h"

Todo_Tab::Todo_Tab(QWidget *parent) :
    QWidget(parent), ui(new Ui::Todo_Tab) {
    ui->setupUi(this);

    connect(ui->up_btn, &QPushButton::clicked, this, [this](){move_up(this);});
    connect(ui->down_btn, &QPushButton::clicked, this, [this](){move_down(this);});

    ui->tab_name->installEventFilter(this);
}

Todo_Tab::Todo_Tab(const QJsonObject& tab_data, QWidget *parent) : Todo_Tab(parent){
    // 解析标题
    if (!tab_data["name"].isString()) {
        qDebug() << "tab name decode error";
        return;
    }
    ui->tab_name->setText(tab_data["name"].toString());

    // 解析条目
    if (!tab_data["todo_entry_list"].isArray()) {
        qDebug() << "tab entries decode error";
        return;
    }
    QJsonArray entry_array = tab_data["todo_entry_list"].toArray();
    for (int i = 0; i < entry_array.size(); i++) {
        if (!entry_array[i].isObject()) {
            qDebug() << "entry decode error";
            return;
        }
        QJsonObject entry_data = entry_array[i].toObject();
        create_entry(entry_data);
    }

    if (!tab_data["Fold"].isBool()) {
        qDebug() << "tab name decode error";
        is_fold=false;
    }else {
        is_fold = tab_data["Fold"].toBool();
    }
    set_fold();

    reorder_entries();
}

Todo_Tab::~Todo_Tab() {
    delete ui;
}

QJsonObject Todo_Tab::get_json() {
    QJsonObject tab_data;
    tab_data["name"] = ui->tab_name->text();
    QJsonArray entry_array;
    for (int i = 0; i < ui->entry_layout->count(); i++){
        Todo_Entry* entry = dynamic_cast<Todo_Entry*>(ui->entry_layout->itemAt(i)->widget());
        entry_array.append(entry->get_json());
    }
    tab_data["todo_entry_list"] = entry_array;
    tab_data["Fold"] = is_fold;

    return tab_data;
}

void Todo_Tab::on_add_btn_clicked() {
    qDebug() << "Todo_Tab::on_add_btn_clicked";

    QJsonObject entry_data;
    entry_data.insert("name", "新条目");
    entry_data.insert("ddl", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm"));
    create_entry(entry_data);

    need_save();
}

void Todo_Tab::create_entry(const QJsonObject &entry_data) {
    // qDebug() << "Todo_Tab::create_entry";
    Todo_Entry* timer_entry = new Todo_Entry(entry_data);
    ui->entry_layout->addWidget(timer_entry);
    connect(timer_entry, &Todo_Entry::need_save, this, &Todo_Tab::need_save);
    connect(timer_entry, &Todo_Entry::need_save, this, &Todo_Tab::reorder_entries);
}

void Todo_Tab::reorder_entries() {
    // qDebug() << "TimerTab::reorder_entries";
    if (ui->entry_layout->isEmpty()) {
        return;
    }

    // 选择排序
    for (int i=0; i<ui->entry_layout->count(); i++) {
        int min_idx = i;
        Todo_Entry* min_entry = dynamic_cast<Todo_Entry*>(ui->entry_layout->itemAt(i)->widget());
        if(!min_entry->ddl_date.isValid()) continue;
        int min_rest = certain_ddl_to_rest_day(min_entry->ddl_date);

        for (int j=i+1; j<ui->entry_layout->count(); j++) {
            Todo_Entry* check_entry = dynamic_cast<Todo_Entry*>(ui->entry_layout->itemAt(j)->widget());
            if(!check_entry->ddl_date.isValid()) continue;

            int rest_day = certain_ddl_to_rest_day(check_entry->ddl_date);
            if (rest_day < 0 && min_rest >= 0) continue;
            if (rest_day >= 0 && min_rest < 0) {
                min_idx = j;
                min_rest = rest_day;
            }
            if (abs(rest_day) < abs(min_rest)) {
                min_idx = j;
                min_rest = rest_day;
            }
        }

        if (i!=min_idx) {
            QLayoutItem *min_item = ui->entry_layout->takeAt(min_idx);
            ui->entry_layout->insertItem(i, min_item);
        }
    }

    if (ui->entry_layout->isEmpty()) {
        ui->next_desc->setText("");
        ui->next_ddl->setText("");
        ui->next_rest->setText("");
        return;
    }

    Todo_Entry* entry = dynamic_cast<Todo_Entry*>(ui->entry_layout->itemAt(0)->widget());
    ui->next_desc->setText(entry->get_desc());
    update_time();
}

void Todo_Tab::update_time() {
    // qDebug() << "TimerTab update_time";

    if (ui->entry_layout->isEmpty()) {
        ui->next_desc->setText("");
        ui->next_ddl->setText("");
        ui->next_rest->setText("");
        return;
    }
    Todo_Entry* entry = dynamic_cast<Todo_Entry*>(ui->entry_layout->itemAt(0)->widget());
    QDate ddl = entry->ddl_date;
    if(!ddl.isValid()){
        ui->next_ddl->setText("");
        ui->next_rest->setText("");
        return;
    }

    ui->next_ddl->setText(ddl.toString("yyyy-MM-dd")+" "+entry->ddl_addon);
    int rest_days = certain_ddl_to_rest_day(ddl);
    QString rest_desc = rest_day_to_desc(rest_days);
    ui->next_rest->setText(rest_desc);
}

void Todo_Tab::on_fold_btn_clicked() {
    is_fold = !is_fold;
    set_fold();
}

void Todo_Tab::set_fold() {
    if(is_fold) {
        ui->entries->hide();
        ui->fold_btn->setIcon(QIcon::fromTheme("zoom-in"));
    }else {
        ui->entries->show();
        ui->fold_btn->setIcon(QIcon::fromTheme("zoom-out"));
    }
}

void Todo_Tab::on_del_btn_clicked() {
    qDebug() << "Todo_Tab::on_del_btn_clicked";
    setParent(NULL);
    need_save();
    deleteLater();
}

bool Todo_Tab::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::FocusOut) {
        // qDebug() << "Child widget lost focus!";
        need_save();
    }
    return QWidget::eventFilter(obj, event);
}
