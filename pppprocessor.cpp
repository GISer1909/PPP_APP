#include "pppprocessor.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QString>
#include <QDateTime>

PPPProcessor::PPPProcessor(QObject *parent)
    : QObject(parent), m_isProcessing(false)
{
    // 初始化默认参数
    memset(&m_paths, 0, sizeof(ppp_paths_t));
    m_paths.mode = MODE_STATIC_PPP;
    m_paths.trace_level = 3;
    
    // 初始化新增的参数
    QDateTime current = QDateTime::currentDateTime();
    
    // 默认处理当天的数据
    m_paths.ts[0] = current.date().year();
    m_paths.ts[1] = current.date().month();
    m_paths.ts[2] = current.date().day();
    m_paths.ts[3] = 0;
    m_paths.ts[4] = 0;
    m_paths.ts[5] = 0;
    
    m_paths.te[0] = current.date().year();
    m_paths.te[1] = current.date().month();
    m_paths.te[2] = current.date().day();
    m_paths.te[3] = 23;
    m_paths.te[4] = 59;
    m_paths.te[5] = 59;
    
    m_paths.ti = 0.0;            // 默认间隔为0，使用观测文件所有数据
    m_paths.niter = 8;           // 默认最大迭代次数
    m_paths.tropopt = TROP_ESTG; // 默认估计对流层延迟和梯度
    m_paths.ionoopt = IONO_IFLC; // 默认电离层无关线性组合
    m_paths.use_time_range = false; // 默认不使用时间范围，处理所有数据
    m_paths.navsys = SYS_GPS | SYS_CMP; // 默认使用GPS和北斗
}

PPPProcessor::~PPPProcessor()
{
    // 确保处理结束时关闭日志
    if (m_isProcessing) {
        traceclose();
    }
}

void PPPProcessor::setMode(run_mode_t mode)
{
    m_paths.mode = mode;
}

QString PPPProcessor::setFilePathWithDoubleBackslashes(const QString &path) {
    QString modifiedPath = path;
    return modifiedPath.replace("/", "\\");
}

void PPPProcessor::setObsFile(const QString &path) {
    QByteArray ba = setFilePathWithDoubleBackslashes(path).toLocal8Bit();
    strncpy(m_paths.obs_file, ba.constData(), sizeof(m_paths.obs_file) - 1);
}

void PPPProcessor::setNavFile(const QString &path) {
    QByteArray ba = setFilePathWithDoubleBackslashes(path).toLocal8Bit();
    strncpy(m_paths.nav_file, ba.constData(), sizeof(m_paths.nav_file) - 1);
}

void PPPProcessor::setSp3File(const QString &path) {
    QByteArray ba = setFilePathWithDoubleBackslashes(path).toLocal8Bit();
    strncpy(m_paths.sp3_file, ba.constData(), sizeof(m_paths.sp3_file) - 1);
}

void PPPProcessor::setClkFile(const QString &path) {
    QByteArray ba = setFilePathWithDoubleBackslashes(path).toLocal8Bit();
    strncpy(m_paths.clk_file, ba.constData(), sizeof(m_paths.clk_file) - 1);
}

void PPPProcessor::setAtxFile(const QString &path) {
    QByteArray ba = setFilePathWithDoubleBackslashes(path).toLocal8Bit();
    strncpy(m_paths.atx_file, ba.constData(), sizeof(m_paths.atx_file) - 1);
}

void PPPProcessor::setDcbFile(const QString &path) {
    QByteArray ba = setFilePathWithDoubleBackslashes(path).toLocal8Bit();
    strncpy(m_paths.dcb_file, ba.constData(), sizeof(m_paths.dcb_file) - 1);
}

void PPPProcessor::setErpFile(const QString &path) {
    QByteArray ba = setFilePathWithDoubleBackslashes(path).toLocal8Bit();
    strncpy(m_paths.erp_file, ba.constData(), sizeof(m_paths.erp_file) - 1);
}

void PPPProcessor::setOutFile(const QString &path) {
    QByteArray ba = setFilePathWithDoubleBackslashes(path).toLocal8Bit();
    strncpy(m_paths.out_file, ba.constData(), sizeof(m_paths.out_file) - 1);
}

void PPPProcessor::setTraceLevel(int level)
{
    m_paths.trace_level = level;
}

void PPPProcessor::setTimeRange(const QDateTime &start, const QDateTime &end)
{
    // 设置开始时间
    m_paths.ts[0] = start.date().year();
    m_paths.ts[1] = start.date().month();
    m_paths.ts[2] = start.date().day();
    m_paths.ts[3] = start.time().hour();
    m_paths.ts[4] = start.time().minute();
    m_paths.ts[5] = start.time().second();
    
    // 设置结束时间
    m_paths.te[0] = end.date().year();
    m_paths.te[1] = end.date().month();
    m_paths.te[2] = end.date().day();
    m_paths.te[3] = end.time().hour();
    m_paths.te[4] = end.time().minute();
    m_paths.te[5] = end.time().second();
}

void PPPProcessor::useTimeRange(bool use)
{
    m_paths.use_time_range = use;
}

void PPPProcessor::setInterval(double interval)
{
    m_paths.ti = interval;
}

void PPPProcessor::setMaxIteration(int niter)
{
    m_paths.niter = niter;
}

void PPPProcessor::setTroposphereOption(trop_opt_t opt)
{
    m_paths.tropopt = opt;
}

void PPPProcessor::setIonosphereOption(iono_opt_t opt)
{
    m_paths.ionoopt = opt;
}

void PPPProcessor::setNavSys(int navsys)
{
    m_paths.navsys = navsys;
}

QString PPPProcessor::getStatusMessage() const
{
    return m_statusMessage;
}

bool PPPProcessor::startProcessing()
{
    if (m_isProcessing) {
        m_statusMessage = "已有处理任务正在运行";
        return false;
    }

    m_isProcessing = true;
    emit processingStarted();
    
    m_statusMessage = "开始PPP处理...";
    emit processingProgress(0, m_statusMessage);
    
    // 初始化日志
    QString logFile = QString("ppp_log_%1.txt").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"));
    QByteArray baLogFile = logFile.toLocal8Bit();
    traceopen(baLogFile.constData());
    tracelevel(m_paths.trace_level);
    
    // 设置精密星历和钟差文件
    setupPreciseFiles();
    
    // 设置PPP选项
    prcopt_t prcopt;
    solopt_t solopt;
    filopt_t filopt;
    setPPPOptions(&prcopt, &solopt, &filopt);
    
    // 执行PPP处理
    emit processingProgress(20, "正在执行PPP计算...");
    int ret = runPPP(&prcopt, &solopt, &filopt);
    
    // 关闭日志
    traceclose();
    
    m_isProcessing = false;
    bool success = (ret == 0);
    if (success) {
        m_statusMessage = "PPP处理成功完成！";
    } else {
        m_statusMessage = QString("PPP处理失败，错误码: %1").arg(ret);
    }
    
    emit processingProgress(100, m_statusMessage);
    emit processingFinished(success);
    
    return success;
}

void PPPProcessor::setupPreciseFiles()
{
    emit processingProgress(5, "检查文件有效性...");
    
    // 检查文件是否存在
    auto checkFile = [](const char* filepath, const char* fileType) -> bool {
        if (filepath[0]) {
            QFile file(filepath);
            if (file.exists()) {
                return true;
            } else {
                qWarning() << "警告: 无法打开" << fileType << "文件:" << filepath;
                return false;
            }
        }
        return false;
    };
    
    // 检查SP3文件
    if (!checkFile(m_paths.sp3_file, "精密星历")) {
        m_paths.sp3_file[0] = '\0';
    }
    
    // 检查CLK文件
    if (!checkFile(m_paths.clk_file, "精密钟差")) {
        m_paths.clk_file[0] = '\0';
    }
    
    // 检查NAV文件
    if (!checkFile(m_paths.nav_file, "导航")) {
        m_paths.nav_file[0] = '\0';
    }
    
    // 检查其他文件
    checkFile(m_paths.obs_file, "观测");
    checkFile(m_paths.atx_file, "天线相位中心");
    checkFile(m_paths.dcb_file, "DCB");
    checkFile(m_paths.erp_file, "地球自转参数");
    
    emit processingProgress(10, "文件检查完成");
}

void PPPProcessor::setPPPOptions(prcopt_t *prcopt, solopt_t *solopt, filopt_t *filopt)
{
    emit processingProgress(15, "配置PPP选项...");
    
    // 初始化为默认值
    *prcopt = prcopt_default;
    *solopt = solopt_default;
    memset(filopt, 0, sizeof(filopt_t));

    // 根据运行模式设置PPP模式
    if (m_paths.mode == MODE_STATIC_PPP) {
        prcopt->mode = PMODE_PPP_STATIC;
        emit processingProgress(16, "处理模式: 静态PPP");
    } else {
        prcopt->mode = PMODE_PPP_KINEMA;
        emit processingProgress(16, "处理模式: 动态PPP");
    }

    // 基本PPP设置
    prcopt->soltype = 0;               // 前向解算
    prcopt->niter = m_paths.niter;     // 最大迭代次数
    emit processingProgress(17, QString("最大迭代次数: %1").arg(m_paths.niter));
    
    // 设置对流层延迟模型
    QString tropModelStr;
    switch(m_paths.tropopt) {
        case TROP_OFF:
            prcopt->tropopt = TROPOPT_OFF;
            tropModelStr = "关闭";
            break;
        case TROP_SAAS:
            prcopt->tropopt = TROPOPT_SAAS;
            tropModelStr = "Saastamoinen模型";
            break;
        case TROP_SBAS:
            prcopt->tropopt = TROPOPT_SBAS;
            tropModelStr = "SBAS模型";
            break;
        case TROP_EST:
            prcopt->tropopt = TROPOPT_EST;
            tropModelStr = "估计ZTD";
            break;
        case TROP_ESTG:
        default:
            prcopt->tropopt = TROPOPT_ESTG;
            tropModelStr = "估计ZTD+梯度";
            break;
    }
    emit processingProgress(18, QString("对流层模型: %1").arg(tropModelStr));
    
    // 设置电离层延迟模型
    QString ionoModelStr;
    switch(m_paths.ionoopt) {
        case IONO_OFF:
            prcopt->ionoopt = IONOOPT_OFF;
            ionoModelStr = "关闭";
            break;
        case IONO_BRDC:
            prcopt->ionoopt = IONOOPT_BRDC;
            ionoModelStr = "广播模型";
            break;
        case IONO_SBAS:
            prcopt->ionoopt = IONOOPT_SBAS;
            ionoModelStr = "SBAS模型";
            break;
        case IONO_IFLC:
            prcopt->ionoopt = IONOOPT_IFLC;
            ionoModelStr = "无电离层组合";
            break;
        case IONO_EST:
            prcopt->ionoopt = IONOOPT_EST;
            ionoModelStr = "估计STEC";
            break;
        case IONO_TEC:
            prcopt->ionoopt = IONOOPT_TEC;
            ionoModelStr = "TEC模型";
            break;
        default:
            prcopt->ionoopt = IONOOPT_IFLC;
            ionoModelStr = "无电离层组合";
            break;
    }
    emit processingProgress(19, QString("电离层模型: %1").arg(ionoModelStr));
    
    prcopt->dynamics = (m_paths.mode == MODE_STATIC_PPP) ? 0 : 1; // 根据模式设置动力学
    prcopt->tidecorr = 2;              // 潮汐改正
    prcopt->modear = 3;                // 固定和保持模糊度
    prcopt->navsys = m_paths.navsys;   // 使用用户选择的卫星系统
    prcopt->posopt[4] = 1;             // 相位偏心改正
    prcopt->posopt[5] = 1;             // 相位缠绕改正
    prcopt->sateph = EPHOPT_PREC;      // 精密星历
    
    // 设置文件选项
    if (m_paths.atx_file[0]) strcpy(filopt->rcvantp, m_paths.atx_file);
    if (m_paths.dcb_file[0]) strcpy(filopt->dcb, m_paths.dcb_file);
    if (m_paths.erp_file[0]) strcpy(filopt->eop, m_paths.erp_file);
    
    // 解算结果输出选项    
    solopt->outopt = 1;                // 输出位置结果
    solopt->outhead = 1;               // 输出头信息
    solopt->outvel = 0;                // 不输出速度
    solopt->sstat = SOLF_STAT;         // 输出状态
    solopt->trace = m_paths.trace_level;// 跟踪级别
    strcpy(solopt->sep, " ");          // 分隔符为空格
}

int PPPProcessor::runPPP(const prcopt_t *prcopt, const solopt_t *solopt, const filopt_t *filopt)
{
    char* infiles[8] = { 0 }; // 最多8个输入文件
    int n = 0, ret;
    
    // 设置处理时间
    gtime_t ts = { 0 }, te = { 0 };
    if (m_paths.use_time_range) {
        // 使用用户设定的时间范围
        ts = epoch2time(m_paths.ts);
        te = epoch2time(m_paths.te);
        emit processingProgress(20, QString("使用设定的时间范围: %1 - %2")
                              .arg(QDateTime(QDate(int(m_paths.ts[0]), int(m_paths.ts[1]), int(m_paths.ts[2])), 
                                    QTime(int(m_paths.ts[3]), int(m_paths.ts[4]), int(m_paths.ts[5]))).toString("yyyy-MM-dd hh:mm:ss"))
                              .arg(QDateTime(QDate(int(m_paths.te[0]), int(m_paths.te[1]), int(m_paths.te[2])), 
                                    QTime(int(m_paths.te[3]), int(m_paths.te[4]), int(m_paths.te[5]))).toString("yyyy-MM-dd hh:mm:ss")));
    } else {
        
        // 默认使用观测文件的全部时间范围
        emit processingProgress(20, "使用观测文件的全部时间范围");
    }
    double ti = m_paths.ti; // 处理间隔
    if (ti > 0.0) {
        emit processingProgress(22, QString("处理间隔: %.1f 秒").arg(ti));
    }

    // 添加输入文件
    if (m_paths.obs_file[0]) {
        infiles[n++] = (char*)m_paths.obs_file;
        emit processingProgress(25, QString("添加观测文件: %1").arg(m_paths.obs_file));
    }

    if (m_paths.nav_file[0]) {
        infiles[n++] = (char*)m_paths.nav_file;
        emit processingProgress(30, QString("添加导航文件: %1").arg(m_paths.nav_file));
    }

    if (m_paths.sp3_file[0]) {
        infiles[n++] = (char*)m_paths.sp3_file;
        emit processingProgress(35, QString("添加精密星历文件: %1").arg(m_paths.sp3_file));
    }

    if (m_paths.clk_file[0]) {
        infiles[n++] = (char*)m_paths.clk_file;
        emit processingProgress(40, QString("添加精密钟差文件: %1").arg(m_paths.clk_file));
    }    // 确保至少有观测文件和导航/精密星历文件
    if (n < 2) {
        m_statusMessage = "错误：需要至少一个观测文件和导航/精密星历文件！";
        emit processingProgress(50, m_statusMessage);
        return -1;
    }
    
    // 记录使用的卫星系统
    QStringList systems;
    if (m_paths.navsys & SYS_GPS) systems << "GPS";
    if (m_paths.navsys & SYS_GLO) systems << "GLONASS";
    if (m_paths.navsys & SYS_GAL) systems << "Galileo";
    if (m_paths.navsys & SYS_CMP) systems << "BeiDou";    if (m_paths.navsys & SYS_QZS) systems << "QZSS";
    if (m_paths.navsys & SYS_IRN) systems << "IRNSS";
    if (m_paths.navsys & SYS_SBS) systems << "SBAS";
    emit processingProgress(45, QString("使用的卫星系统: %1").arg(systems.join(", ")));

    emit processingProgress(50, "开始PPP计算...");

    qDebug() << "ts:" << ts.time;
    qDebug() << "te:" << te.time;
    qDebug() << "ti:" << ti;
    qDebug() << "out_file:" << m_paths.out_file;
    //打印全部参数
    qDebug() << "prcopt:" << prcopt->mode << prcopt->tropopt << prcopt->ionoopt << prcopt->dynamics;
    qDebug() << "solopt:" << solopt->outopt << solopt->outhead << solopt->outvel;
    qDebug() << "filopt:" << filopt->rcvantp << filopt->dcb << filopt->eop;
    qDebug() << "infiles:" << infiles[0] << infiles[1] << infiles[2] << infiles[3];
    qDebug() << "n:" << n;
    
    
    // 执行后处理
    ret = postpos(ts, te, ti, 0.0, prcopt, solopt, filopt, infiles, n, 
                 (char*)m_paths.out_file, (char*)"", (char*)"");

    if (ret == 0) {
        emit processingProgress(90, "PPP处理成功完成");
    } else {
        emit processingProgress(90, QString("PPP处理失败，错误码: %1").arg(ret));
    }

    return ret;
}
