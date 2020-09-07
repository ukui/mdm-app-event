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
    std::multimap<WId, WinData>  m_windowList;
    KWindowSystem*               m_win;
    std::pair<WId, WinData>      m_lastCloseWin;
    WId                          m_oldActiveWin;

    WinData     getInfoByWid(WId);
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
