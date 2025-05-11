#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressBar>
#include <QLabel>
#include <QTableWidget>
#include <QDateTime>
#include "pppprocessor.h"

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

private slots:
    // 文件选择槽函数
    void on_btnSelectObsFile_clicked();
    void on_btnSelectNavFile_clicked();
    void on_btnSelectSp3File_clicked();
    void on_btnSelectClkFile_clicked();
    void on_btnSelectAtxFile_clicked();
    void on_btnSelectDcbFile_clicked();
    void on_btnSelectErpFile_clicked();
    void on_btnSelectOutFile_clicked();
    
    // 处理控制槽函数
    void on_btnStartProcessing_clicked();
    void on_btnClearLog_clicked();
    
    // 结果表格操作槽函数
    void on_btnExportResults_clicked();
    void on_btnClearResults_clicked();
    
    // PPP处理器事件处理
    void onProcessingStarted();
    void onProcessingFinished(bool success);
    void onProcessingProgress(int percent, const QString &message);
    
    // 新增设置选项槽函数
    void on_dateTimeStart_dateTimeChanged(const QDateTime &dateTime);
    void on_dateTimeEnd_dateTimeChanged(const QDateTime &dateTime);
    void on_checkBoxUseTimeRange_toggled(bool checked);
    void on_spinBoxNiter_valueChanged(int value);
    void on_doubleSpinBoxInterval_valueChanged(double interval);
    void on_comboBoxTropModel_currentIndexChanged(int index);
    void on_comboBoxIonoModel_currentIndexChanged(int index);
    
    // 卫星系统选择槽函数
    void on_checkBoxGPS_toggled(bool checked);
    void on_checkBoxGLONASS_toggled(bool checked);
    void on_checkBoxGalileo_toggled(bool checked);
    void on_checkBoxBeiDou_toggled(bool checked);
    void on_checkBoxQZSS_toggled(bool checked);
    void on_checkBoxIRNSS_toggled(bool checked);
    void on_checkBoxSBAS_toggled(bool checked);
    void updateNavSys(); // 更新卫星系统设置
    
    // 菜单操作槽函数
    void on_action_Exit_triggered();
    void on_action_About_triggered();

private:
    Ui::MainWindow *ui;
    PPPProcessor *m_processor;
    QProgressBar *m_progressBar;
    QLabel *m_statusLabel;
    
    // 卫星系统复选框
    QCheckBox *checkBoxGPS;
    QCheckBox *checkBoxGLONASS;
    QCheckBox *checkBoxGalileo;
    QCheckBox *checkBoxBeiDou;
    QCheckBox *checkBoxQZSS;
    QCheckBox *checkBoxIRNSS;
    QCheckBox *checkBoxSBAS;
    
    // 结果数据结构
    struct PPPResult {
        QDateTime timestamp;
        double latitude;
        double longitude;
        double height;
        int quality;
        int numSatellites;
        double sdn;
        double sde;
        double sdu;
        double sdne;
        double sdeu;
        double sdun;
    };
    QList<PPPResult> m_results;
    
    // 辅助函数
    QString selectFile(const QString &title, const QString &filter);
    void updateUIState(bool isProcessing);
    void logMessage(const QString &message);
    bool parseResultFile(const QString &filename);
    void displayResults();
    void setupNavSystemUI(); // 设置卫星系统UI
};
#endif // MAINWINDOW_H
