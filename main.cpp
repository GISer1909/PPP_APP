#include "mainwindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>

int main(int argc, char *argv[])
{    QApplication a(argc, argv);
    
    // 设置应用信息
    QApplication::setApplicationName("精密单点定位程序");
    QApplication::setOrganizationName("GNSS-PPP");
    
    try {
        // 创建主窗口
        MainWindow w;
        w.show();
        
        return a.exec();
    } catch (const std::exception& e) {
        // 捕获并显示标准异常
        QMessageBox::critical(nullptr, "错误", 
                           QString("应用程序遇到了一个致命错误：%1").arg(e.what()));
    } catch (...) {
        // 捕获其他异常
        QMessageBox::critical(nullptr, "错误", 
                           "应用程序遇到了一个未知的致命错误");
    }
    
    return 1;
}
