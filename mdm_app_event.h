#ifndef MDMAPPEVENT_H
#define MDMAPPEVENT_H

#include <map>
#include <unordered_map>
#include <QtCore/QObject>
#include <QtDBus>
#include <KWindowSystem>
#include <memory>
#include <string>

// WinData 包名：UID 的键值对
//typedef std::pair<QString, uint> WinData;
typedef std::string WinData;
typedef std::unordered_map<std::string, std::string> desktops;
typedef std::pair<QString,int> txState;

class MdmAppEvent : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.mdm.app.event")
public:
    explicit MdmAppEvent(QObject *parent = nullptr);
private:
    std::shared_ptr<KWindowSystem> m_win;

    /*!
     * \brief m_windowList
     * 存储窗口id与desktop名的键值对，记录所有创建的窗口,因为关
     * 闭事件发生时，获取到的窗口id是已经被销毁的窗口id
    */
    std::multimap<WId, WinData> m_windowList;

    /*!
     * \brief m_lastCloseWin
     * 记录上一次关闭的窗口，因为关闭的同时也会发出失焦信号
     * 如果在关闭后由于失焦去读取这个窗口的信息显然是不可取的，因此这里记录上
     * 次关闭的窗口信息，如果失焦事件的窗口是上次关闭的窗口就从这个成员中读取
    */
    std::pair<WId, WinData> m_lastCloseWin;

    /*!
     * \brief m_oldActiveWin
     * 记录上一个活动的窗口，失焦和获焦的事件是通过"当前活动
     * 窗口改变"这个信号来判断的，当前有一个窗口获得焦点，那么上一个窗口就会失
     * 去焦点，而这个成员就负责记录那个窗口
    */
    WId m_oldActiveWin;

    /*!
     * \brief TXAppList
     * \typedef std::map<TXAppid, appid>
    */
//    static std::map<std::string, std::string> TXAppList;

    std::string getInfoByWid(const WId&);       // 根据窗口id取得窗口安装包名和UID
    std::string getWinInfo(const WId&);         // 根据窗口id从m_windowList取窗口信息

    /*!
     * \brief getAppName
     * \param pid
     * \details 根据pid获取所需desktop文件名
     * 首先通过pid获取窗口进程的exe和cmdline，使用exe获取对应的安装包名
     * 根据安装包名获取安装包所安装的所有desktop文件
     * 用cmdline和desktop文件中的exec对比，与cmdline一致则为所需desktop文件
     * 无法通过cmdline匹配到的进程为使用脚本启动的进程，如wps
     * 针对这种进程寻找其父进程来获取实际的cmdline并和desktop的exec进行对比
     * 如果都无法获取则证明这个进程的父进程在拉起窗口后已经退出，进行特殊处理
    */
    std::string getAppName(const uint&);

    /*!
     * \brief getPkgName
     * \param 进程启动路径
    */
    std::string getPkgName(const std::string&);

    /*!
     * \brief getAppNameByCmdline
     * \param 某个包安装的desktop文件列表 cmdline
     * \details 对比desktop的exec和cmdline获取所需desktop文件名
    */
    std::string getAppNameByCmdline(const std::vector<std::string>&, const std::string&);

    /*!
     * \brief getAppNameByPPid
     * \param 某个包安装的desktop文件列表 pid
     * \details 根据pid获取相应的父进程，使用父进程的cmdline来对比desktop
    */
    std::string getAppNameByPPid(const std::vector<std::string>&, const uint&);

    /*!
     * \brief getAppNameByTxappid
     * \param 腾讯appid
     * \details 根据腾讯的appid获取所需的desktop文件名
    */
    std::string getAppNameByTxappid(const std::string&);

    /*!
     * \brief getPkgContent
     * \param 安装包名
     * \details 获取安装包安装的所有desktop文件名
    */
    std::vector<std::string> getPkgContent(const std::string&);

//    std::string getAppUid(const std::string&);  // 根据/proc/pid/status获取UID
//    QString     getDesktopNameByPkg(const QString&); // 根据包名获取desktop文件名
//    WId         getWidByDesktop(const std::string&, const uint&); // 根据desktop文件名和UID获取Wid
//    std::string getPyExePath(const uint&);          // 根据进程id获取Python程序的真实启动路径
//    std::string getAppNameByExe(const std::vector<std::string>&, const std::string&);     // 根据启动文件名匹配appid
//    WinData     getInfoByWid(const WId&);       // 根据窗口id取得窗口安装包名和UID
//    WinData     getWinInfo(const WId&);         // 根据窗口id从m_windowList取窗口信息

    //监听 tx 应用名字
//    txState m_txSateChanged;
//    QString m_txOpenedAppName="";
//    QString m_txClosedAppName="";
//    desktops                       m_desktopfps;

Q_SIGNALS:
    /*!
     * \brief 对外发出的dbus信号
     * \param appid:应用的desktop文件名，以/usr/share/applications目录下的为准
     * userid:启动应用的用户id
     */
    void app_open(QString appid);
    void app_close(QString appid);
    void app_minimum(QString appid);
    void app_get_focus(QString appid);
    void app_lose_focus(QString appid);

public Q_SLOTS:
//    QString testMethod(const QString&);
    uint closeApp(QString appid);

private Q_SLOTS:
    void getAddSig(WId);
    void getRemoveSig(WId);
    void getChangeSig(WId,
                      NET::Properties,
                      NET::Properties2);
    void getActiveWinChanged(WId);

    void getTXClosed(QString,QString, int);
    void getTXOpened(QString,QString);
    void getTXStateChanged(QString,QString, int);
};

#endif // MDMAPPEVENT_H
