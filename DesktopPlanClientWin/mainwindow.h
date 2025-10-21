#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <QSettings>
#include <QDateTime>
#include <QUdpSocket>

#include <windows.h>
#include "Monitor.h"
#include "todo_tab.h"
#include "DataMgr.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    HWND hWnd;
    Monitor* monitor;
    DataMgr* data_mgr;

    QSettings* config;

    bool inited = false;

    // 窗口样式相关
private:
    void set_window_style();
    int boundaryWidth = 4; // 可拖动距离
    QPoint m_dragPosition;  // 用于窗口移动的临时变量
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    void paintEvent(QPaintEvent*event) override;

    // 数据相关
public:
    QJsonArray get_json();
    void load_data();
public slots:
    void save_data();

    // 主界面相关
private:
    QFont ft;
public slots:
    void add_tab();
    void create_tab(const QJsonObject &tab_data);
    void move_up(Todo_Tab* tab);
    void move_down(Todo_Tab* tab);

    // 工具栏相关
private:
    bool synced;
    bool saved;
    bool locked;
public slots:
    void on_sync_btn_clicked();
    void on_lock_btn_clicked();
    void on_add_btn_clicked();
    void on_exit_btn_clicked();
};
#endif // MAINWINDOW_H
