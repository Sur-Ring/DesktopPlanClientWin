//
// Created by Jiang_Boyuan on 25-10-3.
//

#ifndef MONITOR_H
#define MONITOR_H

#include <QObject>
#include <QTimer>

#include <windows.h>

class Monitor :public QObject{
    Q_OBJECT
public:
    Monitor(HWND hWnd, int topmost_time);
    ~Monitor();
public:
    // 用于监控桌面置顶
    void installHook();//安装钩子函数
    void unInstallHook();//删除钩子
    HWND desktop;
    HWND WorkerW;
    HWINEVENTHOOK FG_hook; // 钩子对象
    signals:
        void at_desktop();

    // 用于置顶窗口
public:
    void elevate();
private:
    int topmost_time = 50;
    HWND hWnd;
    QTimer *m_pTimer;
public slots:
    void handleTimeout(); //超时处理函数
};

#endif //MONITOR_H
