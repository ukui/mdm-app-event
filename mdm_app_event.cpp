#include <stdlib.h>
#include <unistd.h>
#include <utility>
#include <thread>
// #include <pthread.h>
#include <fstream>
#include <iostream>
#include <string>

#include <QDir>
#include <QString>

#include "mdm_app_event.h"
#include "ukuimenuinterface.h"

#define APP_PATH "/usr/share/applications/"
#define BAD_WINDOW "bad_gay"


MdmAppEvent::MdmAppEvent(QObject *_parent) : QObject(_parent), m_windowList(),
                                                               m_win(KWindowSystem::self()),
                                                               m_oldActiveWin()
                                                               // m_oldActiveWin(m_win->activeWindow())

{    // 注册服务、注册对象
    QDBusConnection::sessionBus().registerService("com.mdm.app.event");
    QDBusConnection::sessionBus().registerObject("/com/mdm/app/event",
                                                 "com.mdm.app.event", this,
                                                 QDBusConnection :: ExportAllSlots |
                                                 QDBusConnection :: ExportAllSignals);

    // 建立连接接受FK5的信号
    connect(m_win, SIGNAL(windowChanged(
                            WId, NET::Properties, NET::Properties2)),
            this, SLOT(getChangeSig(
                           WId, NET::Properties, NET::Properties2)));
    connect(m_win, SIGNAL(windowAdded(WId)), this, SLOT(getAddSig(WId)));
    connect(m_win, SIGNAL(windowRemoved(WId)), this, SLOT(getRemoveSig(WId)));
    connect(m_win, SIGNAL(activeWindowChanged(WId)), this, SLOT(getActiveWinChanged(WId)));

    // 初始化app_map
    initDesktopFps();

    // for test
    // WinData winData = getInfoByWid(m_win->activeWindow());
    // qDebug() << "active window:" << QString(winData.first);
    // m_windowList.insert(std::make_pair(m_win->activeWindow(), winData));
}

//QString MdmAppEvent::testMethod(const QString& arg)
//{
//    return QString("Get interfaceMethod reply: %1").arg(arg);
//}

void MdmAppEvent::getAddSig(WId _id)
{
    // m_win->setState(_id, NET::Max);
    WinData winData = getInfoByWid(_id);
    m_windowList.insert(std::make_pair(_id, winData));
    QString app = getDesktopNameByPkg(winData.first);
    if (app != BAD_WINDOW) {
        qDebug() << "open window:" << app;
        Q_EMIT app_open(app, winData.second);
    }
}

void MdmAppEvent::getRemoveSig(WId _id)
{
    if (m_windowList.find(_id) == m_windowList.end())
        return;
    WinData winData;
    auto    win = m_windowList.find(_id);
    winData = win->second;
    QString app = getDesktopNameByPkg(winData.first);
    if (app != BAD_WINDOW) {
        qDebug() << "close window:" << app;
        Q_EMIT app_close(app, winData.second);
    }
    m_lastCloseWin = *win;
    m_windowList.erase(win);
}

void MdmAppEvent::getActiveWinChanged(WId _id)
{
    // 在窗口层面，窗口活动等于得到焦点
    if(m_windowList.find(_id) != m_windowList.end()) {
        WinData winData = getWinInfo(_id);
        QString app = getDesktopNameByPkg(winData.first);
        if (app != BAD_WINDOW) {
            qDebug() << "get focus:" << app;
            Q_EMIT app_get_focus(app, winData.second);
        }
    }
//    else {
//        m_oldActiveWin = _id;
//    }


    WinData oldWinData;
    if (m_oldActiveWin != NULL && m_oldActiveWin == m_lastCloseWin.first) {
        oldWinData = m_lastCloseWin.second;
        m_lastCloseWin.first = 0;
    }
    else {
        if (m_windowList.find(m_oldActiveWin) == m_windowList.end()) {
            m_oldActiveWin = _id;
            return;
        }
        oldWinData = getWinInfo(m_oldActiveWin);
    }
    QString app = getDesktopNameByPkg(oldWinData.first);
    if (app != BAD_WINDOW) {
        qDebug() << "lose focuse:" << app;
        Q_EMIT app_lose_focus(app, oldWinData.second);
    }
    m_oldActiveWin = _id;
}

void MdmAppEvent::getChangeSig(WId _id,
                               NET::Properties _properties,
                               NET::Properties2 _properties2)
{
    if (m_windowList.find(_id) == m_windowList.end())
        return;
    if(_properties.testFlag(NET::Property::XAWMState)) {
        // 最小化和从最小化变为显示时会发出这个信号
        // 可以判断是否是最小化
        KWindowInfo wininfo = m_win->windowInfo(_id, NET::Property::WMState);
        if(wininfo.isMinimized()){
            WinData winData = getWinInfo(_id);
            QString app = getDesktopNameByPkg(winData.first);
            if (app != BAD_WINDOW) {
                qDebug() << "minimum window:" << app;
                Q_EMIT app_minimum(app, winData.second);
            }
        }
    }
}

uint MdmAppEvent::closeApp(QString app_name, uint userid)
{
    if (userid == 0 || userid < 1000)
        return 1;
    WId toClose = getWidByDesktop(app_name.toStdString(), userid);
    if (toClose != -1)
        m_win->windowRemoved(toClose);
    else
        return 2;

//    // QList<WId> windows = m_win->windows();
//    // 通过KF5提供的KWindowInfo来获取PID
//    std::string pathNameInStd = app_name.toStdString();
//    std::string appNameInStd(pathNameInStd.begin() + pathNameInStd.rfind('/') + 1,
//                             pathNameInStd.end());

//    // 通过遍历桌面所有window的列表来寻找需要的窗口
//    for (auto beg = m_windowList.cbegin(); beg != m_windowList.cend(); ++beg) {
//        if (!m_win->hasWId(beg->first)) {
//            return 3;
//        }
//        KWindowInfo   wininfo = m_win->windowInfo(beg->first, NET::Property::WMState);
//        uint          pid = wininfo.pid();
//        std::string   name;
//        std::string   fileName;
//        std::string   UID;

//        fileName = "/proc/" + std::to_string(pid) + "/status";

//        // 读取应用名
//        // name = getAppName(fileName);
//        name = getAppName(pid);
//        if (name == appNameInStd) {
//            UID = getAppUid(fileName);
//            if (strtoul(UID.c_str(), NULL, 10) == userid) {
//                std::string str = "kill " + std::to_string(pid);
//                // qDebug() << QString::fromStdString(str);
//                if (system(str.c_str()) == 0) {
//                    // windows = m_win->windows();
//                    return 0;
//                }
//                return 3;
//            }
//        }
//    }
//    return 2;
}


// private function

WinData MdmAppEvent::getInfoByWid(const WId& _id)
{
    // 通过KF5提供的KWindowInfo来获取PID
    KWindowInfo wininfo = m_win->windowInfo(_id, NET::Property::WMState);
    uint        pid = wininfo.pid();
    std::string name;

    /*
     * 通过文件流来从文件/proc/pid/status中读取UID和应用名
     * line:按行从文件中读数据，作为行的缓存
     * name:存储窗口所属的应用名称
     * UID:存储用户ID，文本信息取出来是字符串
    */
    std::string   fileName;
    std::string   UID;

    fileName = "/proc/" + std::to_string(pid) + "/status";

    // name = getAppName(fileName);
    name = getAppName(pid);
    UID  = getAppUid(fileName);

    return std::make_pair(QString::fromStdString(name),
                     strtoul(UID.c_str(), NULL, 10));
}

std::string MdmAppEvent::getAppName(const std::string& _fileName)
{
    std::ifstream inFile;
    std::string   line;

    inFile.open(_fileName);
    if(!inFile.is_open()) {
        std::cout << "Can not open file." << std::endl;
        // return;
    }
    // 读取应用名
    getline(inFile, line);
    uint        begin = 5;
    uint        end = line.find("\t", begin + 1);

    return line.substr(begin + 1, (end - (begin + 1)));
}

std::string MdmAppEvent::getAppName(const uint& _pid)
{
    /*
     * 使用这个函数代替同名重载函数获取应用名
     * 需要用到c接口所以看起来可能比较混乱
     * 逻辑：使用readlink从/proc/pid/exe读取窗口进程启动的文件，再通过dpkg -S
     * 获取这个文件所属的安装包
     * pathBuf：用来存储readlink获取到的连接到的路径
     * nameBuf：用来存储fgets获取到的文件流中的数据，这个数据是dpkg -S的返回结果
    */

    char pathBuf[50] = "";
    std::string path = "/proc/" + std::to_string(_pid) +"/exe";
    std::string procPkgName;
    qDebug() << path.c_str();
    if (0 != readlink(path.c_str(), pathBuf, 50)) {
        std::string pathStr = pathBuf;
        return getPkgName(pathStr);
    }
    else {
        qDebug() << "read link file error";
    }
}

std::string MdmAppEvent::getPkgName(const std::string& _fpath)
{
    std::string comm = "dpkg -S " + _fpath;
    qDebug() << QString::fromStdString(comm);
    FILE*       fp = popen(comm.c_str(), "r");
    char        nameBuf[50] = "";
    fgets(nameBuf, 50, fp);
    // std::string packageName = nameBuf;
    pclose(fp);
    std::string pkgInfo = nameBuf;
    return std::string(pkgInfo.begin(),
                       pkgInfo.begin() + pkgInfo.find(':'));
}

std::string MdmAppEvent::getPkgName(const std::string&& _fpath)
{
    std::string comm = "dpkg -S " + _fpath;
    qDebug() << QString::fromStdString(comm);
    FILE*       fp = popen(comm.c_str(), "r");
    char        nameBuf[50] = "";
    fgets(nameBuf, 50, fp);
    pclose(fp);
    std::string pkgInfo = nameBuf;
    // qDebug() << "pkgInfo" << nameBuf;
    if (pkgInfo == "")
        return std::string(BAD_WINDOW);
    return std::string(pkgInfo.begin(),
                       pkgInfo.begin() + pkgInfo.find(':'));
}

std::string MdmAppEvent::getAppUid(const std::string& _fileName)
{
    std::ifstream inFile;
    std::string   line;
    inFile.open(_fileName);
    if(!inFile.is_open())
        std::cout << "Can not open file." << std::endl;

    // 读取UID
    for (int var = 0; var < 9; ++var)
        getline(inFile, line);
    uint begin = 4;
    uint end = line.find("\t", begin + 1);
    return line.substr(begin + 1, (end - (begin + 1)));
}

WinData MdmAppEvent::getWinInfo(const WId& _id)
{
    // 完全使用m_windowList
    // qDebug() << (*m_windowList.find(_id)).first;
    auto win = m_windowList.find(_id);
    return win->second;
}

QString MdmAppEvent::getDesktopNameByPkg(const QString& _pkg)
{
    auto desktopName = m_desktopfps.find(_pkg.toStdString());
    if (desktopName == m_desktopfps.end())
        return BAD_WINDOW;
    std::string name(desktopName->second.begin(), desktopName->second.begin()+desktopName->second.find("."));
    return QString::fromStdString(name);
}

bool MdmAppEvent::initDesktopFps()
{
    QDir dir("/usr/share/applications");
    dir.setFilter(QDir::Dirs|QDir::Files);
    QStringList fileList = dir.entryList();
    int count = fileList.count();
    desktops threadUse[4];

    qDebug() << int(count*0.25) << ":" << int(count*0.5) << ":" << int(0.75*count) << ":" << count;
    std::thread commThread0(&MdmAppEvent::insertDesktops,
                            this, fileList, 0, int(count*0.25), std::ref(threadUse[0]));
    std::thread commThread1(&MdmAppEvent::insertDesktops,
                            this, fileList, int(count*0.25), int(count*0.5), std::ref(threadUse[1]));
    std::thread commThread2(&MdmAppEvent::insertDesktops,
                            this, fileList, int(count*0.5), int(0.75*count), std::ref(threadUse[2]));
    std::thread commThread3(&MdmAppEvent::insertDesktops,
                            this, fileList, int(0.75*count), count, std::ref(threadUse[3]));

    commThread0.join();
    commThread2.join();
    commThread1.join();
    commThread3.join();

    m_desktopfps.insert(threadUse[0].begin(), threadUse[0].end());
    m_desktopfps.insert(threadUse[1].begin(), threadUse[1].end());
    m_desktopfps.insert(threadUse[2].begin(), threadUse[2].end());
    m_desktopfps.insert(threadUse[3].begin(), threadUse[3].end());

    return true;
//    for (auto begin = fileList.begin(); begin != fileList.end(); ++begin) {
//        if (*begin == "." || *begin == "..")
//            continue;
//        if (begin->indexOf(".desktop") == -1)
//            continue;

//        m_desktopfps.insert(std::make_pair(begin->toStdString(),
//                                          getPkgName(std::string(APP_PATH +
//                                                                 begin->toStdString()))));
//    }
}

void MdmAppEvent::insertDesktops(MdmAppEvent* _this, const QStringList& _list, const int& _begin, const int& _end, desktops& _desktops)
{
    qDebug() << _begin << ":" << _end << ":" << _begin + _end;
    for (auto begin = _list.begin()+_begin; begin != _list.begin()+_end; ++begin) {
        // qDebug() << *begin;
        if (*begin == "." || *begin == "..")
            continue;
        if (begin->indexOf(".desktop") == -1)
            continue;

        std:: string pkgName = _this->getPkgName(std::string(APP_PATH + begin->toStdString()));
        if (pkgName != BAD_WINDOW) {
            _desktops.insert(std::make_pair(pkgName, begin->toStdString()));
        }
    }
}

WId MdmAppEvent::getWidByDesktop(const std::string& _desktopName, const uint& _uid)
{
    // app_name是desktop名,根据参数构造WinData，然后在windowList中搜索这个包的名对应的窗口
    WinData winData = std::make_pair(QString::fromStdString(getPkgName(_desktopName)), _uid);
    for (auto begin = m_windowList.begin(); begin != m_windowList.end(); ++begin) {
        if (begin->second == winData) {
           return begin->first;
        }
    }
    return -1;
}
//void* MdmAppEvent::test(void* _this)
//{
//    qDebug() << "666";
//}

void MdmAppEvent::test()
{
    qDebug() << "666";
}
