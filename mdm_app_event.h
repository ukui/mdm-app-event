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
    /*
     * m_lastCloseWin:记录上一次关闭的窗口，因为关闭的同时也会发出失焦信号
     * 如果在关闭后由于失焦去读取这个窗口的信息显然是不可取的，因此这里记录上
     * 次关闭的窗口信息，如果失焦事件的窗口是上次关闭的窗口就从这个成员中读取
     * m_oldActiveWin:记录上一个活动的窗口，失焦和获焦的事件是通过"当前活动
     * 窗口改变"这个信号来判断的，当前有一个窗口获得焦点，那么上一个窗口就会失
     * 去焦点，而这个成员就负责记录那个窗口
     * 程序会记录所有创建的窗口，如果一个没有被记录的窗口发出以下任何事件都会发
     * 生错误
     * m_windowList:存储窗口id与WinData的键值对，记录所有创建的窗口,因为关
     * 闭事件发生时，获取到的窗口id是已经被销毁的窗口id
     * m_desktopfps:存储的是一个键值对，形式为 安装包名：desktop名
     */
    std::shared_ptr<KWindowSystem> m_win;
    std::multimap<WId, WinData>    m_windowList;
    std::pair<WId, WinData>        m_lastCloseWin;
    WId                            m_oldActiveWin;
    desktops                       m_desktopfps;

    //WinData     getInfoByWid(const WId&);       // 根据窗口id取得窗口安装包名和UID
    //WinData     getWinInfo(const WId&);         // 根据窗口id从m_windowList取窗口信息
    std::string getInfoByWid(const WId&);       // 根据窗口id取得窗口安装包名和UID
    std::string getWinInfo(const WId&);         // 根据窗口id从m_windowList取窗口信息

    std::string getAppName(const uint&);        // 根据进程id获取进程的安装包名
    //std::string getAppUid(const std::string&);  // 根据/proc/pid/status获取UID
    std::string getPkgName(const std::string&); // 根据可执行文件路径获取安装包名
    QString     getDesktopNameByPkg(const QString&); // 根据包名获取desktop文件名
    WId         getWidByDesktop(const std::string&, const uint&); // 根据desktop文件名和UID获取Wid
    std::string getPkgNamePy(const uint&);      // 根据进程id获取Python程序的安装包名
    std::string getAppNameByExe(const std::vector<std::string>&, const std::string&);      // 根据启动文件名匹配appid
    std::string getAppNameByExecPath(const std::vector<std::string>&, const std::string&); // 根据desktop中的exec项与启动文件名exe匹配appid
    std::vector<std::string> getPkgContent(const std::string&); // 根据安装包名获取安装包的安装的desktop文件


    //监听 tx 应用名字
    txState m_txSateChanged;
    QString m_txOpenedAppName="";
    QString m_txClosedAppName="";

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

    //监听 tx 应用名字
    void getTXClosed(QString,QString,int);
    void getTXOpened(QString,QString);
    void getTXStateChanged(QString,QString,int);
};

#endif // MDMAPPEVENT_H
