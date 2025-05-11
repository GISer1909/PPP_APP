#ifndef PPPPROCESSOR_H
#define PPPPROCESSOR_H

#include "rtklib.h"
#include <QObject>
#include <QString>

// 处理模式
typedef enum {
    MODE_STATIC_PPP,       // 静态PPP模式
    MODE_KINEMATIC_PPP     // 动态PPP模式
} run_mode_t;

// 对流层延迟模型
typedef enum {
    TROP_OFF,              // 关闭
    TROP_SAAS,             // Saastamoinen模型
    TROP_SBAS,             // SBAS模型
    TROP_EST,              // 估计ZTD
    TROP_ESTG              // 估计ZTD+梯度
} trop_opt_t;

// 电离层延迟模型
typedef enum {
    IONO_OFF,              // 关闭
    IONO_BRDC,             // 广播模型
    IONO_SBAS,             // SBAS模型
    IONO_IFLC,             // 无电离层组合
    IONO_EST,              // 估计STEC
    IONO_TEC               // TEC模型
} iono_opt_t;

// 定义数据路径结构体
typedef struct {
    // 输入文件路径
    char obs_file[1024];   // 观测文件路径
    char nav_file[1024];   // 导航文件路径
    char sp3_file[1024];   // 精密星历文件路径
    char clk_file[1024];   // 精密钟差文件路径
    char atx_file[1024];   // 天线相位中心文件路径
    char dcb_file[1024];   // DCB文件路径
    char erp_file[1024];   // 地球自转参数文件路径

    // 输出文件路径
    char out_file[1024];   // 输出文件路径

    // 处理参数
    run_mode_t mode;       // 处理模式
    int trace_level;       // 日志级别
    
    // 新增处理参数
    double ts[6];          // 开始时间 [年,月,日,时,分,秒]
    double te[6];          // 结束时间 [年,月,日,时,分,秒]
    double ti;             // 处理间隔 (秒)
    int niter;             // 最大迭代次数
    trop_opt_t tropopt;    // 对流层模型选项
    iono_opt_t ionoopt;    // 电离层模型选项
    bool use_time_range;   // 是否使用时间范围
    int navsys;            // 卫星系统选项(SYS_GPS|SYS_GLO|...)
} ppp_paths_t;

class PPPProcessor : public QObject
{
    Q_OBJECT

public:
    explicit PPPProcessor(QObject *parent = nullptr);
    ~PPPProcessor();

    // 设置处理模式
    void setMode(run_mode_t mode);
    
    // 设置输入/输出文件
    void setObsFile(const QString &path);
    void setNavFile(const QString &path);
    void setSp3File(const QString &path);
    void setClkFile(const QString &path);
    void setAtxFile(const QString &path);
    void setDcbFile(const QString &path);
    void setErpFile(const QString &path);
    void setOutFile(const QString &path);
    
    // 设置处理选项
    void setTraceLevel(int level);
    
    // 设置处理时间范围
    void setTimeRange(const QDateTime &start, const QDateTime &end);
    void useTimeRange(bool use); // 是否使用时间范围
    
    // 设置处理间隔
    void setInterval(double interval);
    
    // 设置迭代次数
    void setMaxIteration(int niter);
    
    // 设置对流层模型
    void setTroposphereOption(trop_opt_t opt);
    
    // 设置电离层模型
    void setIonosphereOption(iono_opt_t opt);
    
    // 设置卫星系统
    void setNavSys(int navsys);
      // 执行PPP处理
    bool startProcessing();
    
    // 获取处理进度和状态
    QString getStatusMessage() const;
    
signals:
    // 处理状态信号
    void processingStarted();
    void processingFinished(bool success);
    void processingProgress(int percent, const QString &message);
    
private:
    // 私有实现函数
    void setupPreciseFiles();
    void setPPPOptions(prcopt_t *prcopt, solopt_t *solopt, filopt_t *filopt);
    int runPPP(const prcopt_t *prcopt, const solopt_t *solopt, const filopt_t *filopt);
    QString setFilePathWithDoubleBackslashes(const QString &path);
    
    // 配置和状态变量
    ppp_paths_t m_paths;
    QString m_statusMessage;
    bool m_isProcessing;
};

#endif // PPPPROCESSOR_H
