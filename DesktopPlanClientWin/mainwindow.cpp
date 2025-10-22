#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QPainter>
#include <QMouseEvent>
#include <QPushButton>
#include <QJsonObject> // { }
#include <QJsonArray> // [ ]
#include <QJsonDocument> // 解析Json
#include <QJsonValue> // int float double bool null { } [ ]
#include <QJsonParseError>
#include <QFile>
#include <QFileInfo>
#include <QNetworkDatagram>

#include <Psapi.h>
#include <windowsx.h>

#include "todo_tab.h"

QString data_file_path = "./Data/Data.txt";
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    qDebug() << "StickyNote init";
    ui->setupUi(this);

    hWnd = (HWND)winId();

    config = new QSettings("./Data/cfg.ini", QSettings::IniFormat);

    // 窗口样式相关
    restoreGeometry(config->value("/SN/Geometry","300,300,300,300").toByteArray());
    int topmost_time = config->value("/SN/TopMost", 50).toUInt();
    set_window_style();
    monitor = new Monitor(hWnd, topmost_time);

    // 主窗口相关
    int font_size = config->value("/SN/FontSize", 14).toUInt();
    config->setValue("/SN/FontSize", font_size);
    ft.setPointSize(font_size);

    // 工具栏相关
    synced = false;
    saved = true;
    locked = config->value("/SN/Locked", false).toBool();
    if (locked) {
        ui->lock_btn->setText("Lock");
        ui->lock_btn->setIcon(QIcon::fromTheme("media-record"));
    }else {
        ui->lock_btn->setText("No Lock");
        ui->lock_btn->setIcon(QIcon::fromTheme("media-optical"));
    }

    // 加载数据
    data_mgr = new DataMgr(data_file_path, config);
    load_data();
    connect(data_mgr, &DataMgr::tab_list_changed, this, &MainWindow::load_data);

    inited = true;
}

MainWindow::~MainWindow(){
    save_data();
    config->setValue("/SN/Locked", locked);
    config->setValue("/SN/Geometry", saveGeometry());
    delete data_mgr;
    delete monitor;
    delete config;
    delete ui;
    qDebug() << "StickyNote exit";
}

// 工具栏相关
#pragma region 工具栏相关
void MainWindow::on_sync_btn_clicked() {
    data_mgr->update_tab_list(get_json());
}

void MainWindow::on_lock_btn_clicked() {
    locked = !locked;
    if (locked) {
        ui->lock_btn->setText("Lock");
        ui->lock_btn->setIcon(QIcon::fromTheme("media-record"));
    }else {
        ui->lock_btn->setText("No Lock");
        ui->lock_btn->setIcon(QIcon::fromTheme("media-optical"));
    }
}

void MainWindow::on_add_btn_clicked() {
    add_tab();
}

void MainWindow::on_exit_btn_clicked() {
    qApp->quit();
}
#pragma endregion 工具栏相关

// 数据相关
#pragma region 数据相关
QJsonArray MainWindow::get_json() {
    QJsonArray tab_array;
    for (int i = 0; i < ui->tab_layout->count(); i++){
        Todo_Tab* tab = dynamic_cast<Todo_Tab*>(ui->tab_layout->itemAt(i)->widget());
        tab_array.append(tab->get_json());
    }
    return tab_array;
}

void MainWindow::load_data() {
    qDebug() << "MainWindow load_data";
    QJsonArray tab_list = data_mgr->get_tab_list();

    QLayoutItem *child;
    //每次使用takeAt取了之后，原有的count就会减少，所以每次取下标为0的即可
    while ((child = ui->tab_layout->takeAt(0)) != NULL) {
        if (child->widget()){
            child->widget()->setParent(NULL);
            delete child->widget();
        }
        else if (child->layout()) {
            ui->tab_layout->removeItem(child->layout());
        }
        delete child;
        child = NULL;
    }

    for (int i = 0; i < tab_list.size(); i++) {
        QJsonValue tab = tab_list.at(i);
        if (!tab.isObject()) {
            qDebug() << "tab decode error";
            return;
        }
        QJsonObject tab_data = tab.toObject();
        create_tab(tab_data);
    }
}

void MainWindow::save_data() {
    qDebug() << "MainWindow save_data";
    data_mgr->update_tab_list(get_json());
}
#pragma endregion 数据相关

// 主界面相关
#pragma region 主界面相关
void MainWindow::add_tab() {
    qDebug() << "MainWindow add_tab";

    QJsonObject tab_data;
    tab_data.insert("name", "新项目");
    tab_data.insert("todo_entry_list", QJsonArray());
    tab_data.insert("Fold", false);
    create_tab(tab_data);
}

void MainWindow::create_tab(const QJsonObject &tab_data) {
    Todo_Tab* todo_tab = new Todo_Tab(tab_data);
    ui->tab_layout->addWidget(todo_tab);
    connect(todo_tab, &Todo_Tab::need_save, this, &MainWindow::save_data);
    connect(todo_tab, &Todo_Tab::move_down, this, &MainWindow::move_down);
    connect(todo_tab, &Todo_Tab::move_up, this, &MainWindow::move_up);
}

void MainWindow::move_up(Todo_Tab* tab) {
    qDebug() << "TimerWidget::move_up";
    int index = ui->tab_layout->indexOf(tab); // 获取控件在布局中的索引
    if (index == 0) return;
    QLayoutItem *item = ui->tab_layout->takeAt(index);
    ui->tab_layout->insertItem(index-1, item);
    save_data();
}

void MainWindow::move_down(Todo_Tab* tab) {
    qDebug() << "TimerWidget::move_down";
    int index = ui->tab_layout->indexOf(tab); // 获取控件在布局中的索引
    if (index ==  ui->tab_layout->count()-1) return;
    QLayoutItem *item = ui->tab_layout->takeAt(index);
    ui->tab_layout->insertItem(index+1, item);
    save_data();
}
#pragma endregion 主界面相关

// 窗口样式相关
#pragma region 窗口样式相关
void MainWindow::set_window_style() {
    setWindowFlags(Qt::Tool|Qt::FramelessWindowHint);
    setAttribute(Qt::WA_StyledBackground);      //启用样式背景绘制
    setAttribute(Qt::WA_TranslucentBackground); //背景透明
}

void MainWindow::paintEvent(QPaintEvent*event){
    QPainter p(this);
    p.setBrush(QColor(48, 48, 48, 128));//填充黑色半透明
    p.drawRect(this->rect());//绘制半透明矩形，覆盖整个窗体
    QWidget::paintEvent(event);
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    if (!inited) return;
    if (locked) return;
    // 当鼠标左键按下时记录鼠标的全局坐标与窗口左上角的坐标差
    if (event->button() == Qt::LeftButton) {
        m_dragPosition = event->pos();
        event->accept();
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    if (!inited) return;
    if (locked) return;
    // 当鼠标左键松开时保存当前位置
    if (event->button() == Qt::LeftButton) {
        config->setValue("/SN/Geometry", saveGeometry());
        event->accept();
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (!inited) return;
    if (locked) return;
    // 当鼠标左键被按下时移动窗口
    if (event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

bool MainWindow::nativeEvent(const QByteArray &eventType, void *message, qintptr *result) {
    if (!inited) return false;
    if (locked) return false;
    MSG *msg = (MSG *) message;
    switch (msg->message) {
        case WM_NCHITTEST:
            const auto ratio = devicePixelRatioF();
        int xPos = GET_X_LPARAM(msg->lParam) / ratio - this->frameGeometry().x();
        int yPos = GET_Y_LPARAM(msg->lParam) / ratio - this->frameGeometry().y();
        if (xPos < boundaryWidth && yPos < boundaryWidth) //左上角
            *result = HTTOPLEFT;
        else if (xPos >= width() - boundaryWidth && yPos < boundaryWidth) //右上角
            *result = HTTOPRIGHT;
        else if (xPos < boundaryWidth && yPos >= height() - boundaryWidth) //左下角
            *result = HTBOTTOMLEFT;
        else if (xPos >= width() - boundaryWidth && yPos >= height() - boundaryWidth) //右下角
            *result = HTBOTTOMRIGHT;
        else if (xPos < boundaryWidth) //左边
            *result = HTLEFT;
        else if (xPos >= width() - boundaryWidth) //右边
            *result = HTRIGHT;
        else if (yPos < boundaryWidth) //上边
            *result = HTTOP;
        else if (yPos >= height() - boundaryWidth) //下边
            *result = HTBOTTOM;
        else //其他部分不做处理，返回false，留给其他事件处理器处理
            return false;
        config->setValue("/SN/Geometry", saveGeometry());
        return true;
    }
    return false; //此处返回false，留给其他事件处理器处理
}
#pragma endregion 窗口样式相关
