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

    // 同步相关
    pwd = config->value("/SN/PWD", "默认密码").toString();
    last_sync = config->value("/SN/LastSync", "2025-10-01 00:00:00").toDateTime();
    last_edit = config->value("/SN/LastEdit", "2025-10-01 00:00:00").toDateTime();

    // 加载数据
    load_data();
    inited = true;
}

MainWindow::~MainWindow(){
    save_data();
    config->setValue("/SN/Locked", locked);
    config->setValue("/SN/Geometry", saveGeometry());
    config->setValue("/SN/LastSync", last_sync);
    config->setValue("/SN/LastEdit", last_edit);
    delete config;
    delete ui;
    delete monitor;
    qDebug() << "StickyNote exit";
}

// 工具栏相关
#pragma region 工具栏相关
void MainWindow::on_sync_btn_clicked() {
    sync_data();
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

// 同步相关
#pragma region 同步相关
void MainWindow::sync_data() {


}
#pragma endregion 同步相关

// 数据相关
#pragma region 数据相关
void MainWindow::load_data() {
    qDebug() << "MainWindow load_data";

    // 读取数据文件
    QFileInfo fileInfo(data_file_path);
    qDebug() << "File Path: " << fileInfo.absoluteFilePath();

    QFile file(data_file_path);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qDebug() << "can't open data file!";
        return;
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
        return;
    }
    if (!doc.isArray()) {
        qDebug() << "data decode error";
        return;
    }
    QJsonArray tab_array = doc.array();
    for (int i = 0; i < tab_array.size(); i++) {
        QJsonValue tab = tab_array.at(i);
        if (!tab.isObject()) {
            qDebug() << "tab decode error";
            return;
        }
        QJsonObject tab_data = tab.toObject();
        create_tab(tab_data);
    }
}

void MainWindow::save_data() {
    qDebug() << "TimerWidget save_data";

    QJsonArray tab_array;
    for (int i = 0; i < ui->tab_layout->count(); i++){
        Todo_Tab* tab = dynamic_cast<Todo_Tab*>(ui->tab_layout->itemAt(i)->widget());
        tab_array.append(tab->get_json());
    }

    QJsonDocument doc;
    doc.setArray(tab_array);

    QFile file(data_file_path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qDebug() << "can't open error!";
        return;
    }
    QTextStream stream(&file);
    stream << doc.toJson();
    file.close();

    sync_data();
}
#pragma endregion 数据相关

// 主界面相关
#pragma region 主界面相关
void MainWindow::add_tab() {
    qDebug() << "MainWindow add_tab";

    QJsonObject tab_data;
    tab_data.insert("TabName", "新项目");
    tab_data.insert("Entries", QJsonArray());
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
        move(event->globalPos() - m_dragPosition);
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
