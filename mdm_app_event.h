#ifndef MDMAPPEVENT_H
#define MDMAPPEVENT_H

#include <map>
#include <QtCore/QObject>
#include <QtDBus>
#include <KWindowSystem>

typedef std::pair<QString, uint> WinData;

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
     */
    KWindowSystem*               m_win;
    std::multimap<WId, WinData>  m_windowList;
    std::pair<WId, WinData>      m_lastCloseWin;
    WId                          m_oldActiveWin;

    WinData     getInfoByWid(WId);
    WinData     getWinInfo(WId);
    std::string getAppName(std::ifstream&, std::string);
    std::string getAppUid(std::ifstream&, std::string);
signals:
    void app_open(QString, uint);
    void app_close(QString, uint);
    void app_minimum(QString, uint);
    void app_get_focus(QString, uint);
    void app_lose_focus(QString, uint);

public slots:
//    QString testMethod(const QString&);
    uint closeApp(QString, uint);

private slots:
    void getAddSig(WId);
    void getRemoveSig(WId);
    void getChangeSig(WId,
                      NET::Properties,
                      NET::Properties2);
    void getActiveWinChanged(WId);
};

#endif // MDMAPPEVENT_H
