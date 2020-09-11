#include <unistd.h>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <stdlib.h>

#include <QString>

#include "mdm_app_event.h"

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
    WinData winData = getInfoByWid(_id);
    qDebug() << "open window:" << QString(winData.first);
    m_windowList.insert(std::make_pair(_id, winData));
    emit app_open(winData.first, winData.second);
}

void MdmAppEvent::getRemoveSig(WId _id)
{
    WinData winData;
    auto    win = m_windowList.find(_id);

    winData = win->second;
    qDebug() << "close window:" <<winData.first;
    emit app_close(winData.first, winData.second);
    m_lastCloseWin = *win;
    m_windowList.erase(win);
}

void MdmAppEvent::getActiveWinChanged(WId _id)
{
    if(!m_win->hasWId(_id))
        return;
    // 在窗口层面，窗口活动等于得到焦点
    if (_id != m_oldActiveWin){
        WinData winData = getWinInfo(_id);
        // qDebug() << "get focus:" << QString(winData.first);
        emit app_get_focus(winData.first, winData.second);
    }
    WinData oldWinData;
    if (m_oldActiveWin == m_lastCloseWin.first) {
        oldWinData = m_lastCloseWin.second;
        m_lastCloseWin.first = 0;
    }
    else {
        oldWinData = getWinInfo(m_oldActiveWin);
    }
    // qDebug() << "lose focuse:" << QString(oldWinData.first);
    emit app_lose_focus(oldWinData.first, oldWinData.second);
    m_oldActiveWin = _id;
}

void MdmAppEvent::getChangeSig(WId _id,
                               NET::Properties _properties,
                               NET::Properties2 _properties2)
{
    if(_properties.testFlag(NET::Property::XAWMState)) {
        // 最小化和从最小化变为显示时会发出这个信号
        // 可以判断是否是最小化
        KWindowInfo wininfo = m_win->windowInfo(_id, NET::Property::WMState);
        if(wininfo.isMinimized()){
            WinData winData = getWinInfo(_id);
            // qDebug() << "minimum window:" << QString(winData.first);
            emit app_minimum(winData.first, winData.second);
        }
    }
}

uint MdmAppEvent::closeApp(QString app_name, uint userid)
{
    if (userid == 0 || userid < 1000)
        return 1;

    // QList<WId> windows = m_win->windows();
    // 通过KF5提供的KWindowInfo来获取PID
    std::string pathNameInStd = app_name.toStdString();
    std::string appNameInStd(pathNameInStd.begin() + pathNameInStd.rfind('/') + 1,
                             pathNameInStd.end());

    // 通过遍历桌面所有window的列表来寻找需要的窗口
    // 这里不再整体的Windows列表中遍历，转而到容器中遍历
    for (auto beg = m_windowList.cbegin(); beg != m_windowList.cend(); ++beg) {
        if (!m_win->hasWId(beg->first)) {
            return 3;
        }
        KWindowInfo   wininfo = m_win->windowInfo(beg->first, NET::Property::WMState);
        uint          pid = wininfo.pid();
        std::string   name;
        std::string   fileName;
        std::string   UID;

        fileName = "/proc/" + std::to_string(pid) + "/status";

        // 读取应用名
        // name = getAppName(fileName);
        name = getAppName(pid);
        if (name == appNameInStd) {
            UID = getAppUid(fileName);
            if (strtoul(UID.c_str(), NULL, 10) == userid) {
                std::string str = "kill " + std::to_string(pid);
                // qDebug() << QString::fromStdString(str);
                if (system(str.c_str()) == 0) {
                    // windows = m_win->windows();
                    return 0;
                }
                return 3;
            }
        }
    }
    return 2;
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
     * 因为不是所有应用的名字都等于进程名，因此改用新的实现，这个实现
     * 虽然完成了功能，但是由于需要用到c接口所以看起来可能比较混乱
     * 逻辑：使用readlink从/proc/pid/exe读取窗口进程启动的文件，再通过dpkg -S
     * 获取这个文件所属的安装包
     * pathBuf：用来存储readlink获取到的连接到的路径
     * nameBuf：用来存储fgets获取到的文件流中的数据，这个数据是dpkg -S的返回结果
    */
    char pathBuf[50] = "";
    std::string path = "/proc/" + std::to_string(_pid) +"/exe";
    qDebug() << path.c_str();
    if (0 != readlink(path.c_str(), pathBuf, 50)) {
        // qDebug() << pathBuf;
        std::string pathStr = pathBuf;
//        if (pathStr.find("(") != std::string::npos) {
//            return std::string("nofile");
//        }

        std::string comm = "dpkg -S " + pathStr;
        FILE*       fp = popen(comm.c_str(), "r");
        char        nameBuf[50] = "";

        fgets(nameBuf, 50, fp);
        std::string packageName = nameBuf;
        pclose(fp);
        // qDebug() << "nameBuf:" << nameBuf;
        std::string name (packageName.begin(),
                          packageName.begin() + packageName.find(':'));
        // qDebug() << "name:" << name.c_str();
        return name;
    }
}

std::string MdmAppEvent::getAppUid(const std::string& _fileName)
{
    std::ifstream inFile;
    std::string   line;
    // 这里应该放入函数中，把传参改一下
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
