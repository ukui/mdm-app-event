#ifndef MDMAPPEVENT_H
#define MDMAPPEVENT_H

#include <map>
#include <unordered_map>
#include <QtCore/QObject>
#include <QtDBus>
#include <KWindowSystem>

// WinData 包名：UID 的键值对
typedef std::pair<QString, uint> WinData;
typedef std::unordered_map<std::string, std::string> desktops;

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
    KWindowSystem*              m_win;
    std::multimap<WId, WinData> m_windowList;
    std::pair<WId, WinData>     m_lastCloseWin;
    WId                         m_oldActiveWin;
    desktops                    m_desktopfps;

    WinData     getInfoByWid(const WId&);       // 根据窗口id取得窗口安装包名和UID
    WinData     getWinInfo(const WId&);         // 根据窗口id从m_windowList取窗口信息
    std::string getAppName(const uint&);        // 根据进程id获取进程的安装包名
    std::string getAppUid(const std::string&);  // 根据/proc/pid/status获取UID
    std::string getPkgName(const std::string&); // 根据可执行文件路径获取安装包名
    std::string getPkgName(const std::string&&);
    QString     getDesktopNameByPkg(const QString&); // 根据包名获取desktop文件名
    WId         getWidByDesktop(const std::string&, const uint&); // 根据desktop文件名和UID获取Wid

    // 多线程遍历/usr/share/applications，并记录所有desktop文件的安装包名，存储在容器m_desktopfps中
    bool        initDesktopFps();
    static void insertDesktops(MdmAppEvent*, const QStringList&, const int&, const int&, desktops&);

Q_SIGNALS:
    void app_open(QString, uint);
    void app_close(QString, uint);
    void app_minimum(QString, uint);
    void app_get_focus(QString, uint);
    void app_lose_focus(QString, uint);

public Q_SLOTS:
//    QString testMethod(const QString&);
    uint closeApp(QString, uint);

private Q_SLOTS:
    void getAddSig(WId);
    void getRemoveSig(WId);
    void getChangeSig(WId,
                      NET::Properties,
                      NET::Properties2);
    void getActiveWinChanged(WId);
};

#endif // MDMAPPEVENT_H
