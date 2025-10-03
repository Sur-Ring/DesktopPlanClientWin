//
// Created by Jiang_Boyuan on 25-10-3.
//

#ifndef TODO_TAB_H
#define TODO_TAB_H

#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui { class Todo_Tab; }
QT_END_NAMESPACE

class Todo_Tab : public QWidget {
Q_OBJECT

public:
    explicit Todo_Tab(QWidget *parent = nullptr);
    explicit Todo_Tab(const QJsonObject &tab_data, QWidget *parent = nullptr);
    ~Todo_Tab() override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    QJsonObject get_json();

    bool is_fold;
    void set_fold();
    void create_entry(const QJsonObject &entry_data);
public slots:
    void update_time();
    void on_fold_btn_clicked();
    void on_add_btn_clicked();
    void on_del_btn_clicked();
    void reorder_entries();
signals:
    void _on_update_time();
    void need_save();
    void move_down(Todo_Tab* tab);
    void move_up(Todo_Tab* tab);
private:
    Ui::Todo_Tab *ui;
};


#endif //TODO_TAB_H
