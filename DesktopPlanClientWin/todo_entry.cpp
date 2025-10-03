//
// Created by Jiang_Boyuan on 25-10-3.
//

// You may need to build the project (run Qt uic code generator) to get "ui_Todo_Entry.h" resolved

#include "todo_entry.h"
#include "ui_Todo_Entry.h"

#include <QScrollBar>
#include <QTextBlock>
#include <QJsonObject> // { }
#include <QJsonArray> // [ ]
#include <QJsonDocument> // 解析Json
#include <QJsonValue> // int float double bool null { } [ ]
#include <QJsonParseError>

#include "DDL_Parser.h"

Todo_Entry::Todo_Entry(QWidget *parent) :
    QWidget(parent), ui(new Ui::Todo_Entry) {
    ui->setupUi(this);

    ui->desc->installEventFilter(this);
    ui->ddl->installEventFilter(this);

    on_ddl_textChanged();
}

Todo_Entry::Todo_Entry(const QJsonObject &entry_data, QWidget *parent) : Todo_Entry(parent){
    // 解析描述
    if (!entry_data["Desc"].isString()) {
        qDebug() << "entry desc decode error";
        return;
    }
    ui->desc->setPlainText(entry_data["Desc"].toString());

    // 解析DDL
    if (!entry_data["DDL"].isString()) {
        qDebug() << "entry desc decode error";
        return;
    }
    ui->ddl->setText(entry_data["DDL"].toString());
}

Todo_Entry::~Todo_Entry() {
    delete ui;
}

QJsonObject Todo_Entry::get_json() {
    QJsonObject entry_data;
    entry_data["Desc"] = ui->desc->toPlainText();
    entry_data["DDL"] = ui->ddl->text();
    return entry_data;
}

QString Todo_Entry::get_desc() {
    return ui->desc->toPlainText();
}

void Todo_Entry::on_ddl_textChanged() {
    // qDebug() << "Todo_Entry on_ddl_textChanged";

    QStringList ddl_parts = ui->ddl->text().split(' ');
    QDate certain_ddl = uncertain_ddl_to_certain_ddl(ddl_parts[0]);
    int rest_days = certain_ddl_to_rest_day(certain_ddl);
    QString rest_desc = rest_day_to_desc(rest_days);
    ui->rest->setText(rest_desc);
    ddl_date = certain_ddl;
    if (ddl_parts.size() > 1)
        ddl_addon = ddl_parts[1];
    else
        ddl_addon = "";
    // edited();
}

void Todo_Entry::on_desc_textChanged() {
    // qDebug() << "TimerEntry _on_desc_changed";
    resize_desc();
    // edited();
}

void Todo_Entry::on_del_btn_clicked() {
    qDebug() << "Todo_Entry::on_del_btn_clicked";
    setParent(NULL);
    need_save();
    deleteLater();
}

void Todo_Entry::resize_desc() {
    // qDebug() << "Todo_Entry resize_desc";

    QTextDocument* doc = ui->desc->document();
    // 计算所有文本块的总高度
    int totalHeight = 0;
    QTextBlock block = doc->firstBlock();
    while (block.isValid()) {
        QRectF blockRect = doc->documentLayout()->blockBoundingRect(block);
        totalHeight += blockRect.height();
        block = block.next();
    }

    // 添加文档边距和控件边距
    int docMargin = doc->documentMargin() * 2;
    int widgetMargin = ui->desc->contentsMargins().top() + ui->desc->contentsMargins().bottom();
    int newHeight = totalHeight + docMargin + widgetMargin;

    // 设置最小高度（至少显示一行）
    if (doc->lineCount() == 0) {
        newHeight = ui->desc->fontMetrics().lineSpacing() + docMargin + widgetMargin;
    }

    // 应用高度并更新布局
    ui->desc->setFixedHeight(newHeight);
    ui->desc->updateGeometry();
}

void Todo_Entry::resizeEvent(QResizeEvent* event) {
    // 判断宽度是否变化
    if (event->oldSize().width() != event->size().width()) {
        resize_desc();
    }
}

bool Todo_Entry::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::FocusOut) {
        // qDebug() << "TimerEntry Child widget lost focus!";
        need_save();
    }
    return QWidget::eventFilter(obj, event);
}
