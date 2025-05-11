#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QDesktopServices>
#include <QUrl>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QRegularExpression>
#include <QClipboard>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{    ui->setupUi(this);
    
    // 设置窗口标题
    setWindowTitle("精密单点定位程序");
    
    // 创建PPP处理器
    m_processor = new PPPProcessor(this);    // 连接信号与槽
    connect(m_processor, &PPPProcessor::processingStarted, this, &MainWindow::onProcessingStarted);
    connect(m_processor, &PPPProcessor::processingFinished, this, &MainWindow::onProcessingFinished);
    connect(m_processor, &PPPProcessor::processingProgress, this, &MainWindow::onProcessingProgress);
    
    // 创建状态栏组件
    m_statusLabel = new QLabel("就绪");
    m_progressBar = new QProgressBar();
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setFixedWidth(200);
    
    // 添加到状态栏
    statusBar()->addWidget(m_statusLabel, 1);
    statusBar()->addPermanentWidget(m_progressBar);
    
    // 初始化界面状态
    updateUIState(false);
      // 日志欢迎信息
    logMessage("精密单点定位程序已启动，请配置参数并选择输入文件。");
    
    // 设置日期时间控件默认值
    QDateTime currentDateTime = QDateTime::currentDateTime();
    ui->dateTimeStart->setDateTime(currentDateTime);
    ui->dateTimeEnd->setDateTime(currentDateTime.addSecs(24*60*60)); // 默认结束时间为当前时间加一天
      // 设置模型下拉框默认值
    ui->comboBoxTropModel->setCurrentIndex(4); // 默认选择估计ZTD+梯度
    ui->comboBoxIonoModel->setCurrentIndex(3); // 默认选择无电离层组合
    
    // 绑定参数变化时的信号
    connect(ui->checkBoxUseTimeRange, &QCheckBox::toggled, ui->dateTimeStart, &QDateTimeEdit::setEnabled);
    connect(ui->checkBoxUseTimeRange, &QCheckBox::toggled, ui->dateTimeEnd, &QDateTimeEdit::setEnabled);
    
    // 添加并设置卫星系统选择组件
    setupNavSystemUI();
}void MainWindow::setupNavSystemUI()
{
    // 创建卫星系统选择组框
    QGroupBox *groupBoxNavSys = new QGroupBox("卫星系统选择");
    QVBoxLayout *navSysLayout = new QVBoxLayout(groupBoxNavSys);
    
    // 创建复选框
    checkBoxGPS = new QCheckBox("GPS (美国)");
    checkBoxGLONASS = new QCheckBox("GLONASS (俄罗斯)");
    checkBoxGalileo = new QCheckBox("Galileo (欧盟)");
    checkBoxBeiDou = new QCheckBox("BeiDou (中国)");
    checkBoxQZSS = new QCheckBox("QZSS (日本)");
    checkBoxIRNSS = new QCheckBox("IRNSS (印度)");
    checkBoxSBAS = new QCheckBox("SBAS (卫星增强系统)");
    
    // 添加到布局
    navSysLayout->addWidget(checkBoxGPS);
    navSysLayout->addWidget(checkBoxGLONASS);
    navSysLayout->addWidget(checkBoxGalileo);
    navSysLayout->addWidget(checkBoxBeiDou);
    navSysLayout->addWidget(checkBoxQZSS);
    navSysLayout->addWidget(checkBoxIRNSS);
    navSysLayout->addWidget(checkBoxSBAS);
    
    // 将导航系统组添加到选项组的网格布局中
    QGridLayout *optionsLayout = qobject_cast<QGridLayout*>(ui->groupBoxOptions->layout());
    if (optionsLayout) {
        // 获取当前的行数，添加到下一行
        int row = optionsLayout->rowCount();
        optionsLayout->addWidget(groupBoxNavSys, row, 0, 1, 2); // 跨两列
    }
    
    // 默认选中GPS和北斗系统
    checkBoxGPS->setChecked(true);
    checkBoxBeiDou->setChecked(true);
    
    // 连接信号槽
    connect(checkBoxGPS, &QCheckBox::toggled, this, &MainWindow::on_checkBoxGPS_toggled);
    connect(checkBoxGLONASS, &QCheckBox::toggled, this, &MainWindow::on_checkBoxGLONASS_toggled);
    connect(checkBoxGalileo, &QCheckBox::toggled, this, &MainWindow::on_checkBoxGalileo_toggled);
    connect(checkBoxBeiDou, &QCheckBox::toggled, this, &MainWindow::on_checkBoxBeiDou_toggled);
    connect(checkBoxQZSS, &QCheckBox::toggled, this, &MainWindow::on_checkBoxQZSS_toggled);
    connect(checkBoxIRNSS, &QCheckBox::toggled, this, &MainWindow::on_checkBoxIRNSS_toggled);
    connect(checkBoxSBAS, &QCheckBox::toggled, this, &MainWindow::on_checkBoxSBAS_toggled);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 文件选择槽函数
void MainWindow::on_btnSelectObsFile_clicked()
{
    QString file = selectFile("选择观测文件", "观测文件 (*.*o *.obs *.rnx);;所有文件 (*)");
    if (!file.isEmpty()) {
        ui->lineEditObsFile->setText(file);
    }
}

void MainWindow::on_btnSelectNavFile_clicked()
{
    QString file = selectFile("选择导航文件", "导航文件 (*.n *.nav *.rnx);;所有文件 (*)");
    if (!file.isEmpty()) {
        ui->lineEditNavFile->setText(file);
    }
}

void MainWindow::on_btnSelectSp3File_clicked()
{
    QString file = selectFile("选择精密星历文件", "SP3文件 (*.sp3);;所有文件 (*)");
    if (!file.isEmpty()) {
        ui->lineEditSp3File->setText(file);
    }
}

void MainWindow::on_btnSelectClkFile_clicked()
{
    QString file = selectFile("选择精密钟差文件", "CLK文件 (*.clk);;所有文件 (*)");
    if (!file.isEmpty()) {
        ui->lineEditClkFile->setText(file);
    }
}

void MainWindow::on_btnSelectAtxFile_clicked()
{
    QString file = selectFile("选择天线相位中心文件", "ATX文件 (*.atx);;所有文件 (*)");
    if (!file.isEmpty()) {
        ui->lineEditAtxFile->setText(file);
    }
}

void MainWindow::on_btnSelectDcbFile_clicked()
{
    QString file = selectFile("选择DCB文件", "DCB文件 (*.dcb *.bsx);;所有文件 (*)");
    if (!file.isEmpty()) {
        ui->lineEditDcbFile->setText(file);
    }
}

void MainWindow::on_btnSelectErpFile_clicked()
{
    QString file = selectFile("选择地球自转参数文件", "ERP文件 (*.erp);;所有文件 (*)");
    if (!file.isEmpty()) {
        ui->lineEditErpFile->setText(file);
    }
}

void MainWindow::on_btnSelectOutFile_clicked()
{
    QString file = QFileDialog::getSaveFileName(this, "选择输出文件", "", 
                                             "位置文件 (*.pos);;所有文件 (*)");
    if (!file.isEmpty()) {
        ui->lineEditOutFile->setText(file);
    }
}

void MainWindow::on_btnStartProcessing_clicked()
{
    // 检查必要的输入文件
    if (ui->lineEditObsFile->text().isEmpty()) {
        QMessageBox::warning(this, "缺少文件", "必须指定观测文件");
        return;
    }
    
    if (ui->lineEditNavFile->text().isEmpty() && ui->lineEditSp3File->text().isEmpty()) {
        QMessageBox::warning(this, "缺少文件", "必须指定导航文件或精密星历文件");
        return;
    }
    
    if (ui->lineEditOutFile->text().isEmpty()) {
        QMessageBox::warning(this, "缺少文件", "必须指定输出文件");
        return;
    }
    
    // 设置PPP处理器参数
    m_processor->setObsFile(ui->lineEditObsFile->text());
    m_processor->setNavFile(ui->lineEditNavFile->text());
    m_processor->setSp3File(ui->lineEditSp3File->text());
    m_processor->setClkFile(ui->lineEditClkFile->text());
    m_processor->setAtxFile(ui->lineEditAtxFile->text());
    m_processor->setDcbFile(ui->lineEditDcbFile->text());
    m_processor->setErpFile(ui->lineEditErpFile->text());
    m_processor->setOutFile(ui->lineEditOutFile->text());
    
    // 设置处理模式
    if (ui->radioButtonStatic->isChecked()) {
        m_processor->setMode(MODE_STATIC_PPP);
    } else {
        m_processor->setMode(MODE_KINEMATIC_PPP);
    }
    
    // 设置日志级别
    m_processor->setTraceLevel(ui->comboBoxTraceLevel->currentIndex() + 1);
    
    // 设置新增的参数
    // 时间范围设置
    if (ui->checkBoxUseTimeRange->isChecked()) {
        m_processor->useTimeRange(true);
        m_processor->setTimeRange(ui->dateTimeStart->dateTime(), ui->dateTimeEnd->dateTime());
    } else {
        m_processor->useTimeRange(false);
    }
    
    // 处理间隔设置
    m_processor->setInterval(ui->doubleSpinBoxInterval->value());
    
    // 迭代次数设置
    m_processor->setMaxIteration(ui->spinBoxNiter->value());
    
    // 对流层模型设置
    trop_opt_t tropOpt = TROP_ESTG;
    switch(ui->comboBoxTropModel->currentIndex()) {
        case 0: tropOpt = TROP_OFF; break;
        case 1: tropOpt = TROP_SAAS; break;
        case 2: tropOpt = TROP_SBAS; break;
        case 3: tropOpt = TROP_EST; break;
        case 4: tropOpt = TROP_ESTG; break;
        default: tropOpt = TROP_ESTG; break;
    }
    m_processor->setTroposphereOption(tropOpt);
    
    // 电离层模型设置
    iono_opt_t ionoOpt = IONO_IFLC;
    switch(ui->comboBoxIonoModel->currentIndex()) {
        case 0: ionoOpt = IONO_OFF; break;
        case 1: ionoOpt = IONO_BRDC; break;
        case 2: ionoOpt = IONO_SBAS; break;
        case 3: ionoOpt = IONO_IFLC; break;
        case 4: ionoOpt = IONO_EST; break;
        case 5: ionoOpt = IONO_TEC; break;
        default: ionoOpt = IONO_IFLC; break;
    }
    m_processor->setIonosphereOption(ionoOpt);
    
    // 更新卫星系统设置
    updateNavSys();
    
    // 开始处理
    m_processor->startProcessing();
}

void MainWindow::on_btnClearLog_clicked()
{
    ui->textEditLog->clear();
    logMessage("日志已清空");
}

void MainWindow::onProcessingStarted()
{
    updateUIState(true);
    logMessage("精密单点定位处理已开始");
}

void MainWindow::onProcessingFinished(bool success)
{
    updateUIState(false);    if (success) {
        logMessage("精密单点定位处理成功完成！");
        
        // 解析结果文件并显示在表格中
        bool parseSuccess = parseResultFile(ui->lineEditOutFile->text());
        if (parseSuccess) {
            displayResults();
            ui->tabWidget->setCurrentWidget(ui->tabResults);
            logMessage("结果已加载到结果表格中");
        }
        
        // 询问是否打开结果文件
        QMessageBox::StandardButton reply = QMessageBox::question(this, 
                                         "处理完成", 
                                         "精密单点定位处理已成功完成，是否打开结果文件？",
                                         QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(ui->lineEditOutFile->text()));
        }    } else {
        logMessage("处理失败: " + m_processor->getStatusMessage());
        QMessageBox::critical(this, "处理失败", "精密单点定位处理失败: " + m_processor->getStatusMessage());
    }
}

void MainWindow::onProcessingProgress(int percent, const QString &message)
{
    m_progressBar->setValue(percent);
    m_statusLabel->setText(message);
    logMessage(message);
}

QString MainWindow::selectFile(const QString &title, const QString &filter)
{
    return QFileDialog::getOpenFileName(this, title, "", filter);
}

void MainWindow::updateUIState(bool isProcessing)
{
    // 处理期间禁用UI控件
    ui->groupBoxInput->setEnabled(!isProcessing);
    ui->groupBoxOptions->setEnabled(!isProcessing);
    ui->btnStartProcessing->setEnabled(!isProcessing);
    
    if (!isProcessing) {
        m_progressBar->setValue(0);
        m_statusLabel->setText("就绪");
    }
}

void MainWindow::logMessage(const QString &message)
{
    ui->textEditLog->append(QDateTime::currentDateTime().toString("[yyyy-MM-dd hh:mm:ss] ") + message);
}

// 解析结果文件
bool MainWindow::parseResultFile(const QString &filename)
{
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        logMessage("无法打开结果文件：" + filename);
        return false;
    }

    m_results.clear();
    QTextStream in(&file);
    bool dataStarted = false;
    
    while (!in.atEnd()) {
        QString line = in.readLine();
        
        // 跳过注释行
        if (line.startsWith("%")) {
            continue;
        }
        
        // 找到数据行
        QRegularExpression re("^(\\d{4}/\\d{2}/\\d{2}\\s+\\d{2}:\\d{2}:\\d{2}\\.\\d+)\\s+([-+]?\\d+\\.\\d+)\\s+([-+]?\\d+\\.\\d+)\\s+([-+]?\\d+\\.\\d+)\\s+(\\d+)\\s+(\\d+)\\s+([-+]?\\d+\\.\\d+)\\s+([-+]?\\d+\\.\\d+)\\s+([-+]?\\d+\\.\\d+)\\s+([-+]?\\d+\\.\\d+)\\s+([-+]?\\d+\\.\\d+)\\s+([-+]?\\d+\\.\\d+).*$");
        QRegularExpressionMatch match = re.match(line);
        
        if (match.hasMatch()) {
            PPPResult result;
            result.timestamp = QDateTime::fromString(match.captured(1), "yyyy/MM/dd hh:mm:ss.zzz");
            result.latitude = match.captured(2).toDouble();
            result.longitude = match.captured(3).toDouble();
            result.height = match.captured(4).toDouble();
            result.quality = match.captured(5).toInt();
            result.numSatellites = match.captured(6).toInt();
            result.sdn = match.captured(7).toDouble();
            result.sde = match.captured(8).toDouble();
            result.sdu = match.captured(9).toDouble();
            result.sdne = match.captured(10).toDouble();
            result.sdeu = match.captured(11).toDouble();
            result.sdun = match.captured(12).toDouble();
            
            m_results.append(result);
            dataStarted = true;
        } else if (dataStarted) {
            // 如果已经开始解析数据但当前行不匹配，可能是格式变化，跳过
            logMessage("警告：跳过不匹配的行: " + line);
        }
    }
    
    file.close();
    logMessage(QString("成功解析结果文件，共找到 %1 条数据记录").arg(m_results.size()));
    return !m_results.isEmpty();
}

// 在表格中显示结果
void MainWindow::displayResults()
{
    ui->tableWidgetResults->setRowCount(0);
    
    for (const PPPResult &result : m_results) {
        int row = ui->tableWidgetResults->rowCount();
        ui->tableWidgetResults->insertRow(row);
        
        // 添加数据到表格
        ui->tableWidgetResults->setItem(row, 0, new QTableWidgetItem(result.timestamp.toString("yyyy/MM/dd hh:mm:ss.zzz")));
        ui->tableWidgetResults->setItem(row, 1, new QTableWidgetItem(QString::number(result.latitude, 'f', 9)));
        ui->tableWidgetResults->setItem(row, 2, new QTableWidgetItem(QString::number(result.longitude, 'f', 9)));
        ui->tableWidgetResults->setItem(row, 3, new QTableWidgetItem(QString::number(result.height, 'f', 4)));
        
        // 质量指示器说明
        QString qualityText;
        switch (result.quality) {
            case 1: qualityText = "1-固定解"; break;
            case 2: qualityText = "2-浮点解"; break;
            case 3: qualityText = "3-SBAS"; break;
            case 4: qualityText = "4-DGPS"; break;
            case 5: qualityText = "5-单点定位"; break;
            case 6: qualityText = "6-PPP"; break;
            default: qualityText = QString::number(result.quality);
        }
        ui->tableWidgetResults->setItem(row, 4, new QTableWidgetItem(qualityText));
        
        ui->tableWidgetResults->setItem(row, 5, new QTableWidgetItem(QString::number(result.numSatellites)));
        ui->tableWidgetResults->setItem(row, 6, new QTableWidgetItem(QString::number(result.sdn, 'f', 4)));
        ui->tableWidgetResults->setItem(row, 7, new QTableWidgetItem(QString::number(result.sde, 'f', 4)));
        ui->tableWidgetResults->setItem(row, 8, new QTableWidgetItem(QString::number(result.sdu, 'f', 4)));
        ui->tableWidgetResults->setItem(row, 9, new QTableWidgetItem(QString::number(result.sdne, 'f', 4)));
        ui->tableWidgetResults->setItem(row, 10, new QTableWidgetItem(QString::number(result.sdeu, 'f', 4)));
        ui->tableWidgetResults->setItem(row, 11, new QTableWidgetItem(QString::number(result.sdun, 'f', 4)));
    }
    
    // 自动调整列宽以适应内容
    ui->tableWidgetResults->resizeColumnsToContents();
}

// 导出结果按钮槽函数
void MainWindow::on_btnExportResults_clicked()
{
    if (m_results.isEmpty()) {
        QMessageBox::warning(this, "导出失败", "没有可导出的结果数据");
        return;
    }
    
    QString exportPath = QFileDialog::getSaveFileName(this, "导出结果数据", 
                                                    QFileInfo(ui->lineEditOutFile->text()).path() + "/结果数据.csv", 
                                                    "CSV文件 (*.csv);;所有文件 (*)");
    
    if (exportPath.isEmpty()) {
        return;
    }
    
    QFile file(exportPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "导出失败", "无法创建导出文件：" + exportPath);
        return;
    }
    
    QTextStream out(&file);
    
    // 写入CSV头
    out << "时间,纬度(度),经度(度),高程(m),解算质量,卫星数,sdn(m),sde(m),sdu(m),sdne(m),sdeu(m),sdun(m)\n";
    
    // 写入数据
    for (const PPPResult &result : m_results) {
        out << result.timestamp.toString("yyyy/MM/dd hh:mm:ss.zzz") << ","
            << QString::number(result.latitude, 'f', 9) << ","
            << QString::number(result.longitude, 'f', 9) << ","
            << QString::number(result.height, 'f', 4) << ","
            << result.quality << ","
            << result.numSatellites << ","
            << QString::number(result.sdn, 'f', 4) << ","
            << QString::number(result.sde, 'f', 4) << ","
            << QString::number(result.sdu, 'f', 4) << ","
            << QString::number(result.sdne, 'f', 4) << ","
            << QString::number(result.sdeu, 'f', 4) << ","
            << QString::number(result.sdun, 'f', 4) << "\n";
    }
    
    file.close();
    logMessage("结果数据已成功导出到: " + exportPath);
    QMessageBox::information(this, "导出成功", "结果数据已成功导出到: " + exportPath);
}

// 清空结果按钮槽函数
void MainWindow::on_btnClearResults_clicked()
{
    if (m_results.isEmpty() && ui->tableWidgetResults->rowCount() == 0) {
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "清空结果", 
                                                           "确定要清空所有结果数据吗？",
                                                           QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        m_results.clear();
        ui->tableWidgetResults->setRowCount(0);
        logMessage("已清空结果数据");
    }
}

// 时间范围设置
void MainWindow::on_checkBoxUseTimeRange_toggled(bool checked)
{
    m_processor->useTimeRange(checked);
    if (checked) {
        QDateTime startTime = ui->dateTimeStart->dateTime();
        QDateTime endTime = ui->dateTimeEnd->dateTime();
        m_processor->setTimeRange(startTime, endTime);
    }
}

void MainWindow::on_dateTimeStart_dateTimeChanged(const QDateTime &dateTime)
{
    if (ui->checkBoxUseTimeRange->isChecked()) {
        m_processor->setTimeRange(dateTime, ui->dateTimeEnd->dateTime());
    }
}

void MainWindow::on_dateTimeEnd_dateTimeChanged(const QDateTime &dateTime)
{
    if (ui->checkBoxUseTimeRange->isChecked()) {
        m_processor->setTimeRange(ui->dateTimeStart->dateTime(), dateTime);
    }
}

// 处理间隔设置
void MainWindow::on_doubleSpinBoxInterval_valueChanged(double interval)
{
    m_processor->setInterval(interval);
}

// 迭代次数设置
void MainWindow::on_spinBoxNiter_valueChanged(int value)
{
    m_processor->setMaxIteration(value);
}

// 对流层模型设置
void MainWindow::on_comboBoxTropModel_currentIndexChanged(int index)
{
    trop_opt_t tropOpt = TROP_ESTG; // 默认值
    
    switch(index) {
        case 0: tropOpt = TROP_OFF; break;
        case 1: tropOpt = TROP_SAAS; break;
        case 2: tropOpt = TROP_SBAS; break;
        case 3: tropOpt = TROP_EST; break;
        case 4: tropOpt = TROP_ESTG; break;
        default: tropOpt = TROP_ESTG; break;
    }
    
    m_processor->setTroposphereOption(tropOpt);
}

// 电离层模型设置
void MainWindow::on_comboBoxIonoModel_currentIndexChanged(int index)
{
    iono_opt_t ionoOpt = IONO_IFLC; // 默认值
    
    switch(index) {
        case 0: ionoOpt = IONO_OFF; break;
        case 1: ionoOpt = IONO_BRDC; break;
        case 2: ionoOpt = IONO_SBAS; break;
        case 3: ionoOpt = IONO_IFLC; break;
        case 4: ionoOpt = IONO_EST; break;
        case 5: ionoOpt = IONO_TEC; break;
        default: ionoOpt = IONO_IFLC; break;
    }
    
    m_processor->setIonosphereOption(ionoOpt);
}

// 更新卫星系统设置
void MainWindow::updateNavSys()
{
    int navsys = 0;
    if (checkBoxGPS->isChecked())     navsys |= SYS_GPS;
    if (checkBoxGLONASS->isChecked()) navsys |= SYS_GLO;
    if (checkBoxGalileo->isChecked()) navsys |= SYS_GAL;
    if (checkBoxBeiDou->isChecked())  navsys |= SYS_CMP;
    if (checkBoxQZSS->isChecked())    navsys |= SYS_QZS;
    if (checkBoxIRNSS->isChecked())   navsys |= SYS_IRN;
    if (checkBoxSBAS->isChecked())    navsys |= SYS_SBS;
    
    // 确保至少选择了一个系统
    if (navsys == 0) {
        navsys = SYS_GPS; // 默认至少使用GPS
        checkBoxGPS->setChecked(true);
        QMessageBox::warning(this, "设置提示", "至少需要选择一个卫星系统！已自动选择GPS系统。");
    }
    
    m_processor->setNavSys(navsys);
    
    // 记录到日志
    QStringList systems;
    if (navsys & SYS_GPS) systems << "GPS";
    if (navsys & SYS_GLO) systems << "GLONASS";
    if (navsys & SYS_GAL) systems << "Galileo";
    if (navsys & SYS_CMP) systems << "BeiDou";
    if (navsys & SYS_QZS) systems << "QZSS";
    if (navsys & SYS_IRN) systems << "IRNSS";
    if (navsys & SYS_SBS) systems << "SBAS";
    
    logMessage(QString("已设置卫星系统：%1").arg(systems.join(", ")));
}

// 卫星系统选择槽函数
void MainWindow::on_checkBoxGPS_toggled(bool checked)
{
    updateNavSys();
}

void MainWindow::on_checkBoxGLONASS_toggled(bool checked)
{
    updateNavSys();
}

void MainWindow::on_checkBoxGalileo_toggled(bool checked)
{
    updateNavSys();
}

void MainWindow::on_checkBoxBeiDou_toggled(bool checked)
{
    updateNavSys();
}

void MainWindow::on_checkBoxQZSS_toggled(bool checked)
{
    updateNavSys();
}

void MainWindow::on_checkBoxIRNSS_toggled(bool checked)
{
    updateNavSys();
}

void MainWindow::on_checkBoxSBAS_toggled(bool checked)
{
    updateNavSys();
}

// 菜单操作 - 退出
void MainWindow::on_action_Exit_triggered()
{
    close();
}

// 菜单操作 - 关于
void MainWindow::on_action_About_triggered()
{
    QMessageBox aboutBox(this);
    aboutBox.setWindowTitle("关于");
    aboutBox.setIcon(QMessageBox::Information);
    
    QString aboutText = "<h3>精密单点定位程序</h3>";
    aboutText += "<p>版本: 1.0.0</p>";
    aboutText += "<p>本程序支持利用精密星历和钟差产品进行高精度GNSS单点定位。</p>";
    aboutText += "<p>支持的卫星系统：GPS、GLONASS、Galileo、BeiDou、QZSS、IRNSS、SBAS</p>";
    aboutText += "<hr/>";
    aboutText += "<p><b>由GISer1909制作，仅供学习交流。All rights reserved.</b></p>";
    
    aboutBox.setText(aboutText);
    aboutBox.setStandardButtons(QMessageBox::Ok);
    aboutBox.exec();
}
