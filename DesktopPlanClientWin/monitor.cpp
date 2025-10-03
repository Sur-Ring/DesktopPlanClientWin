//
// Created by Jiang_Boyuan on 25-10-3.
//

#include "monitor.h"
#include <QDebug>
#include <winsock2.h>
#include <windows.h>

static Monitor *monitor; //Qt界面中调用Hook类的对象

void CALLBACK WinEventProc(
    HWINEVENTHOOK hWinEventHook,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD dwEventThread,
    DWORD dwmsEventTime
) {
    if (hwnd && idObject == OBJID_WINDOW && idChild == CHILDID_SELF && event == EVENT_SYSTEM_FOREGROUND) {
        HWND hWnd = GetForegroundWindow();
        // qDebug() << "hwnd"<< hwnd <<" front:" << hWnd << " desktop:" << monitor->desktop << "WorkerW:" << monitor->WorkerW;

        if (hWnd == monitor->desktop || hWnd == monitor->WorkerW) {
            emit monitor->at_desktop();
            monitor->elevate();
            return;
        }

        TCHAR className[256];
        int length = GetClassName(hwnd, className, 256);
        QString className_ = QString(className);
        // qDebug() << "className:" << className<<" className_:" << className_<<" equal:"<<(className_ == "WorkerW");
        if (className_ == "WorkerW" || className_ == "WorkerA" || className_ == "Progman" || className_ == "SHELLDLL_DefView") {
            emit monitor->at_desktop();
            monitor->elevate();
        }
    }
}

Monitor::Monitor(HWND hWnd, int topmost_time) {
    qDebug() << "Monitor init";

    desktop = FindWindow(LPCWSTR(QString("Progman").utf16()), LPCWSTR(QString("Program Manager").utf16()));
    WorkerW = FindWindow(LPCWSTR(QString("WorkerW").utf16()), LPCWSTR(QString("").utf16()));
    FG_hook = nullptr;
    installHook();

    this->hWnd = hWnd;
    this->topmost_time = topmost_time;
    m_pTimer = new QTimer(this);
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(handleTimeout()));

    elevate();

    qDebug() << "desktop is: " << desktop;
}

Monitor::~Monitor() {
    qDebug() << "Monitor destroy";
    unInstallHook();
}

void Monitor::installHook() {
    //安装钩子函数
    FG_hook = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND,
        NULL, WinEventProc,
        0, 0,
        WINEVENT_OUTOFCONTEXT); // WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS

    monitor = this;
    qDebug() << "install hook";
}

void Monitor::unInstallHook() {
    //删除钩子函数
    UnhookWinEvent(FG_hook);
    FG_hook = nullptr;
    monitor = nullptr;
    qDebug() << "uninstall hook";
}

void Monitor::elevate() {
    SetWindowPos(
        hWnd, // 目标窗口句柄
        HWND_TOPMOST, // 置于所有窗口之上
        0, 0, 0, 0, // 忽略位置和大小参数
        SWP_NOMOVE | SWP_NOSIZE // 保持当前位置和大小
    );
    m_pTimer->start(topmost_time);
}

void Monitor::handleTimeout() {
    // qDebug() << "Elevator: unset topmost";
    SetWindowPos(
        hWnd,
        HWND_NOTOPMOST, // 恢复为普通窗口
        0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE
    );
    m_pTimer->stop();
}