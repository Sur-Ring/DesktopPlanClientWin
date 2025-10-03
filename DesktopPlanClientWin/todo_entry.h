//
// Created by Jiang_Boyuan on 25-10-3.
//

#ifndef TODO_ENTRY_H
#define TODO_ENTRY_H

#include <QWidget>
#include <QDateTime>


QT_BEGIN_NAMESPACE
namespace Ui { class Todo_Entry; }
QT_END_NAMESPACE

class Todo_Entry : public QWidget {
Q_OBJECT

public:
    explicit Todo_Entry(QWidget *parent = nullptr);
    explicit Todo_Entry(const QJsonObject &entry_data, QWidget *parent = nullptr);
    ~Todo_Entry() override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    QJsonObject get_json();
    QString get_desc();
    QDate ddl_date;
    QString ddl_addon = "";
protected:
    void resizeEvent(QResizeEvent* event) override;
public slots:
    void on_desc_textChanged();
    void resize_desc();
    void on_ddl_textChanged();
    void on_del_btn_clicked();
signals:
    void need_save();
private:
    Ui::Todo_Entry *ui;
};


#endif //TODO_ENTRY_H
