#include <stdlib.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <string>

#include <QDir>
#include <QString>

#include <gio/gdesktopappinfo.h>

#include "mdm_app_event.h"

#define APP_PATH "/usr/share/applications/"

std::map<std::string, std::string>
MdmAppEvent::TXAppList = {
                            {"0779b2dd64bb4d2ea4dfcb48dbb8c491", "tencent-chinese-composition-correction"},
                            {"2f4a6a9071b642ee6eb72f55de74506e", "tencent-course-center"},
                            {"38651128f1384d07a69ca184e6f9ccb6", "tencent-english-composition-correction"},
                            {"558f8d31c2524e68292bccd8cda17d23", "tencent-homework"},
                            {"06c71dbf873c46229d66cde9c4e8f18e", "tencent-precise-practice"},
                            {"b12f7283c27f4e48a7bd9510587f3c42", "tencent-translator"}
                         };

MdmAppEvent::MdmAppEvent(QObject *_parent) : QObject(_parent), m_win(KWindowSystem::self()),
                                                               m_windowList(),
                                                               m_oldActiveWin(0)

{
    m_lastCloseWin = std::make_pair(0, std::string());

    // 注册服务、注册对象
    //QDBusConnection::systemBus().unregisterService("com.mdm.app.event");
    QDBusConnection::sessionBus().unregisterService("com.mdm.app.event");
    if (!QDBusConnection::sessionBus().registerService("com.mdm.app.event"))
        qWarning() << "register dbus service error!";
    if (!QDBusConnection::sessionBus().registerObject("/com/mdm/app/event",
                                                     "com.mdm.app.event", this,
                                                     QDBusConnection :: ExportAllSlots |
                                                     QDBusConnection :: ExportAllSignals))
        qWarning() << "register object error;";


    // 建立连接接受FK5的信号
    connect(m_win.get(), SIGNAL(windowChanged(
                           WId, NET::Properties, NET::Properties2)),
                           this, SLOT(getChangeSig(
                           WId, NET::Properties, NET::Properties2)));
    connect(m_win.get(), SIGNAL(windowAdded(WId)), this, SLOT(getAddSig(WId)));

    connect(m_win.get(), SIGNAL(windowRemoved(WId)), this, SLOT(getRemoveSig(WId)));
    connect(m_win.get(), SIGNAL(activeWindowChanged(WId)), this, SLOT(getActiveWinChanged(WId)));


    QDBusConnection::sessionBus().connect("cn.txeduplatform.server",
                                              "/cn/txeduplatform/server",
                                              "cn.txeduplatform.server.apps",
                                              "OnClosed", this,
                                              SLOT(getTXClosed(QString,QString,int)));


    //监听腾讯应用的信号
    QDBusConnection::sessionBus().connect("cn.txeduplatform.server",
                                              "/cn/txeduplatform/server",
                                              "cn.txeduplatform.server.apps",
                                              "OnOpened", this,
                                              SLOT(getTXOpened(QString,QString)));

    QDBusConnection::sessionBus().connect("cn.txeduplatform.server",
                                              "/cn/txeduplatform/server",
                                              "cn.txeduplatform.server.apps",
                                              "OnStateChanged", this,
                                              SLOT(getTXStateChanged(QString,QString,int)));
    // for test
    // WinData winData = getInfoByWid(m_win->activeWindow());
    // qDebug() << "active window:" << QString(winData.first);
    // m_windowList.insert(std::make_pair(m_win->activeWindow(), winData));
}

void MdmAppEvent::getTXClosed(QString TxAppid,QString TxAppname,int lifetime)
{
    QString appid = QString::fromStdString(getAppNameByTxappid(TxAppid.toStdString()));
    if (!appid.isEmpty()) {
        qDebug() << "close window:" << appid;
        Q_EMIT app_close(appid);
    }

}

void MdmAppEvent::getTXOpened(QString TxAppid,QString TxAppname)
{
    QString appid = QString::fromStdString(getAppNameByTxappid(TxAppid.toStdString()));
    if (!appid.isEmpty()) {
        qDebug() << "open window:" << appid;
        Q_EMIT app_open(appid);
    }
}

void MdmAppEvent::getTXStateChanged(QString TxAppid,QString TxAppname,int state)
{
    if(state == 1)
    {
        QString appid = QString::fromStdString(getAppNameByTxappid(TxAppid.toStdString()));
        if (!appid.isEmpty()) {
            qDebug() << "minimum window:" << appid;
            Q_EMIT app_minimum(appid);
        }
    }
    else if(state == 5)
    {
        QString appid = QString::fromStdString(getAppNameByTxappid(TxAppid.toStdString()));
        if (!appid.isEmpty()) {
            qDebug() << "lose focus:" << appid;
            Q_EMIT app_lose_focus(appid);
        }
    }
    else if(state == 6)
    {
        QString appid = QString::fromStdString(getAppNameByTxappid(TxAppid.toStdString()));
        if (!appid.isEmpty()) {
            qDebug() << "get focus:" << appid;
            Q_EMIT app_get_focus(appid);
        }
    }
}

void MdmAppEvent::getAddSig(WId _id)
{
    std::string winData = getInfoByWid(_id);
    if (winData.empty())
        return;

    m_windowList.insert(std::make_pair(_id, winData));

    if(winData != "txeduplatform-runtime")
    {
        qDebug() << "open window:" << QString::fromStdString(winData);
        //Q_EMIT app_open(winData.first, winData.second);
        Q_EMIT app_open(QString::fromStdString(winData));
    }
}

void MdmAppEvent::getRemoveSig(WId _id)
{
    std::string winData;
    auto    win = m_windowList.find(_id);
    if (win == m_windowList.end()) {
        m_lastCloseWin = std::make_pair(0, std::string());
    }
    else {
        winData = win->second;

        if(winData != "txeduplatform-runtime")
        {
            Q_EMIT app_close(QString::fromStdString(winData));
            qDebug() << "close window:" << QString::fromStdString(winData);
            m_lastCloseWin = *win;
            m_windowList.erase(win);
        }
    }
}

void MdmAppEvent::getActiveWinChanged(WId _id)
{
    // 在窗口层面，窗口活动等于得到焦点
    if(m_windowList.find(_id) != m_windowList.end()) {
        std::string winData = getWinInfo(_id);
        // QString app = getDesktopNameByPkg(winData.first);
        if(winData != "txeduplatform-runtime")
        {
            qDebug() << "get focus:" << QString::fromStdString(winData);
            Q_EMIT app_get_focus(QString::fromStdString(winData));
        }
    }

    std::string oldWinData;
    if (m_oldActiveWin != 0 && m_oldActiveWin == m_lastCloseWin.first) {
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

    // QString app = getDesktopNameByPkg(oldWinData.first);
    if (oldWinData.length()!=0) {
        if(oldWinData != "txeduplatform-runtime")
        {
            qDebug() << "lose focuse:" << QString::fromStdString(oldWinData);
            Q_EMIT app_lose_focus(QString::fromStdString(oldWinData));
        }
    }
    m_oldActiveWin = _id;
}

void MdmAppEvent::getChangeSig(WId _id,
                               NET::Properties _properties,
                               NET::Properties2 _properties2)
{
    Q_UNUSED(_properties2)
    if (m_windowList.find(_id) == m_windowList.end())
        return;
    if(_properties.testFlag(NET::Property::XAWMState)) {
        // 最小化和从最小化变为显示时会发出这个信号
        // 可以判断是否是最小化
        KWindowInfo wininfo(_id, NET::Property::WMState);
        if(wininfo.isMinimized()){
            std::string winData = getWinInfo(_id);
            // QString app = getDesktopNameByPkg(winData.first);
            if(winData != "txeduplatform-runtime")
            {
                qDebug() << "minimum window:" << QString::fromStdString(winData);
                Q_EMIT app_minimum(QString::fromStdString(winData));
            }
        }
    }
}

uint MdmAppEvent::closeApp(QString appid)
{

    bool isFind = false;
    std::multimap<WId, WinData> list = m_windowList;
    for (auto begin = list.begin(); begin != list.end(); ++begin) {
        // qDebug() << begin->first;
        if (begin->second == appid.toStdString()) {
            uint pid = KWindowInfo(begin->first, NET::Property::WMState).pid();
            // qDebug() << pid;
            system(std::string("kill " + std::to_string(pid)).c_str());
            isFind = true;
        }
    }
    if (isFind){
        return 0;
    }
    else
        return 2;
}

std::string MdmAppEvent::getInfoByWid(const WId& _id)
{
    // 通过KF5提供的KWindowInfo来获取PID
    KWindowInfo wininfo(_id, NET::Property::WMState);
    if (!wininfo.valid()) {
        return std::string();
    }
    uint        pid = wininfo.pid();
    std::string name;

    name = getAppName(pid);
    //UID  = getAppUid(fileName);

    if ( name.empty()) {
        return std::string();
    }
    //return std::make_pair(QString::fromStdString(name),
                     //strtoul(UID.c_str(), NULL, 10));
    return name;
}

std::string MdmAppEvent::getAppName(const uint& _pid)
{
    std::ifstream inFile;
    std::string   cmdline;
    std::string   procCmdline = "/proc/" + std::to_string(_pid) + "/cmdline";
    std::string   procExe = "/proc/" + std::to_string(_pid) + "/exe";
    // qDebug() << path.c_str();

    inFile.open(procCmdline);
    if (!inFile.is_open()) {
        qWarning() << "Open file:" << procCmdline.c_str() << " failed";
        return std::string();
    }
    getline(inFile, cmdline);
    inFile.close();

    char pathBuf[100] = "";
    if (0 != readlink(procExe.c_str(), pathBuf, 100)) {
        if (strlen(pathBuf) == 0)
            return std::string();
    }
    else {
        qWarning() << "Get link:" << procExe.c_str() << " failed";
        return std::string();
    }
    std::string exe = pathBuf;

    if (cmdline.find("/usr/bin/python") != std::string::npos) {
        // 删除Python程序的Python解释器和位于Python与真实路径之间的\0
        if (cmdline.find('\x0') != std::string::npos)
            cmdline = std::string(cmdline.begin() + cmdline.find('\x0')+1,
                       cmdline.end());
        exe = cmdline;
    }
    // 一般情况下在cmdline的结尾也会有一个\0
    if (cmdline.find('\x0') != std::string::npos)
        cmdline = std::string(cmdline.begin(), cmdline.begin() + cmdline.find('\x0'));

    std::string pkg = getPkgName(exe);
    if (pkg.empty()) {
        qWarning() << QString::fromStdString(cmdline) + "can not find install packaeg";
        return std::string();
    }

    std::vector<std::string> desktopName = getPkgContent(pkg);
    if (desktopName.empty()) {
        qWarning() << "MdmAppEvent::getAppName this install package:"
                   << QString::fromStdString(pkg)
                   << " with no desktop file";
        return std::string();
    }

    if (desktopName.size() == 1) {
        // 如果只有一个desktop文件，万事大吉
        return desktopName[0];
    }

    // 处理cmdline和desktop文件中启动路径相同的
    std::string appid = getAppNameByCmdline(desktopName, cmdline);
    if (!appid.empty())
        return appid;

    // 无法通过cmdline和desktop中的启动路径直接匹配的认为是脚本拉起的
    appid = getAppNameByPPid(desktopName, _pid);
    qDebug() << appid.c_str();
    if (!appid.empty())
        return appid;

    qWarning() << "Can not find appid by cmdline:" << cmdline.c_str();
    return std::string();

#if 0
    appid = getAppNameByExe(desktopName, pathStr);
    if (!appid.empty())
        return appid;

    char pathBuf[100] = "";
    std::string   path = "/proc/" + std::to_string(_pid) +"/exe";
    if (0 != readlink(path.c_str(), pathBuf, 100)) {
        if (strlen(pathBuf) == 0)
            return std::string();

        std::string pathStr = pathBuf;

        if (pathStr.find("/usr/bin/python") != std::string::npos)
            pathStr = getPyExePath(_pid);

        std::string pkg = getPkgName(pathStr);
        if (pkg.empty()) {
            qWarning() << QString::fromStdString(pathStr) + "can not find install packaeg";
            return std::string();
        }
        std::vector<std::string> desktopName = getPkgContent(pkg);
        if (desktopName.empty()) {
            qWarning() << "MdmAppEvent::getAppName this install package:" + QString::fromStdString(pkg) + "with no desktop file";
            return std::string();
        }
        if (desktopName.size() == 1) {
            // 如果只有一个desktop文件，万事大吉
            return desktopName[0];
        }

        std::string appid = getAppNameByExecPath(desktopName, pathStr);
        if (!appid.empty())
            return appid;

        appid = getAppNameByExe(desktopName, pathStr);
        if (!appid.empty())
            return appid;

        // 以上办法都不能匹配到就找一个最短的，同时给一个警告
        std::string name = desktopName[0];
        for (auto begin = desktopName.begin(); begin != desktopName.end(); ++begin) {
            if (begin->size() < name.size())
                name = *begin;
        }
        qWarning() << "getAppName：this app:" + QString::fromStdString(name) + " may not get the ture name";
        return name;
    }
    else {
        qWarning() << "read the file: " + QString::fromStdString(path) + " link error";
        return "";
    }
#endif
}

std::string MdmAppEvent::getPkgName(const std::string& _fpath)
{
    std::string path;
    if (_fpath.find(" ") != std::string::npos)
        path = std::string(_fpath.begin(), _fpath.begin() + _fpath.find(" "));
    else
        path = _fpath;

    // 针对软件商店进行的特殊处理
    if (path.find("./") != std::string::npos &&
        path.find("kylin-software") != std::string::npos)
        path = "/usr/share" +
                std::string(path.begin() + path.find("./") + 1, path.begin() + path.find(".py")) +
                std::string(path.begin() + path.find("./") + 1, path.end());

    std::string comm = "dpkg -S " + path;
    FILE*       fp = popen(comm.c_str(), "r");
    char        nameBuf[100] = "";
    fgets(nameBuf, 100, fp);
    pclose(fp);

    std::string pkgInfo = nameBuf;
    if (pkgInfo.empty()) {
        return std::string();
    }

    return std::string(pkgInfo.begin(),
                       pkgInfo.begin() + pkgInfo.find(':'));
}

std::string MdmAppEvent::getWinInfo(const WId& _id)
{
    auto win = m_windowList.find(_id);
    return win->second;
}


std::vector<std::string> MdmAppEvent::getPkgContent(const std::string& _pkgname)
{
    std::string comm = "dpkg -L " + _pkgname;
    FILE*       fp = popen(comm.c_str(), "r");
    char        buff[255];
    if (fp == NULL) {
        qWarning() << "getPkgContent: dpkg -L error, pkg name:" << _pkgname.c_str();
        return std::vector<std::string>();
    }
    std::vector<std::string> desktopName;
    for (int i = 0; fgets(buff, 255, fp) != NULL; ++i) {
        std::string line = buff;
        if (line.find(".desktop") != std::string::npos &&
            line.find("/usr/share/applications/") != std::string::npos) {
            desktopName.push_back(std::string(line.begin()+line.rfind("/")+1,
                                              line.begin()+line.rfind(".desktop")));
        }
    }
    pclose(fp);
    return desktopName;
}


std::string MdmAppEvent::getAppNameByCmdline(const std::vector<std::string>& _desktops, const std::string& _cmdline)
{
    for (auto begin = _desktops.begin(); begin != _desktops.end(); ++begin) {
        std::string path = "/usr/share/applications/" + *begin + ".desktop";

        GDesktopAppInfo *desktopInfo =
                g_desktop_app_info_new_from_filename(path.c_str());
        if (!desktopInfo) {
            qWarning() << "GDesktopAppInfo:get file failed:" << begin->c_str();
            continue;
        }
        std::string exec = g_desktop_app_info_get_string(desktopInfo, "Exec");
        if (exec.find("%") != std::string::npos)
            exec = std::string(exec.begin(), exec.begin() + exec.find("%") - 1);

        g_object_unref(desktopInfo);
        if (_cmdline == exec)
            return *begin;
    }
    qDebug() << "cmdline matching failed, cmdline:" << _cmdline.c_str();
    return std::string();
}

std::string MdmAppEvent::getAppNameByTxappid(const std::string& _tx_appip)
{
    auto iterator = this->TXAppList.find(_tx_appip);
    if (iterator != TXAppList.end()) {
        return iterator->second;
    }
    else {
        // 如果应用是未记录的腾讯应用，则在applications中搜索所有腾讯应用并逐一匹配
        QDir         applications("/usr/share/applications");
        QStringList  filter;
        filter << "tencent-*";

        QList<QFileInfo> fileInfos = applications.entryInfoList(filter);
        for (auto begin = fileInfos.begin(); begin != fileInfos.end(); ++begin) {
            GDesktopAppInfo *desktopInfo =
                    g_desktop_app_info_new_from_filename(begin->absoluteFilePath().toUtf8());
            if (desktopInfo) {
                auto appid = g_desktop_app_info_get_string(desktopInfo, "Appid");
                if (appid && _tx_appip == appid) {
                    std::string fileName = begin->fileName().toStdString();

                    g_object_unref(desktopInfo);
                    g_free(appid);

                    return std::string(fileName.begin(), fileName.begin() + fileName.rfind("."));
                }
                g_object_unref(desktopInfo);
                g_free(appid);
            }
        }
        return std::string();
    }
}

std::string MdmAppEvent::getAppNameByPPid(const std::vector<std::string> &_desktops, const uint& _pid)
{
    std::string   statusPath = "/proc/" + std::to_string(_pid) + "/status";
    std::ifstream inFile;
    std::string   line;
    inFile.open(statusPath);
    if(!inFile.is_open()) {
        qWarning() << "Can not open file:" << statusPath.c_str();
        return std::string();
    }

    for (int var = 0; var < 7; ++var)
        getline(inFile, line);
    inFile.close();

    uint begin = 5;
    uint end = line.find("\t", begin + 1);
    std::string ppid = line.substr(begin + 1, (end - (begin + 1)));
    std::string cmdlinePath = "/proc/" + ppid + "/cmdline";
    std::string cmdline;

    inFile.open(cmdlinePath);
    if (!inFile.is_open()) {
        qWarning() << "Open file:" << cmdlinePath.c_str() << " failed";
        return std::string();
    }
    getline(inFile, cmdline);
    inFile.close();

    if (cmdline.find("bash") != std::string::npos) {
        // 删除Python程序的Python解释器和位于Python与真实路径之间的\0
        if (cmdline.find('\x0') != std::string::npos)
            cmdline = std::string(cmdline.begin() + cmdline.find('\x0')+1,
                       cmdline.end());
    }
    // 一般情况下在cmdline的结尾也会有一个\0
    if (cmdline.find('\x0') != std::string::npos)
        cmdline = std::string(cmdline.begin(), cmdline.begin() + cmdline.find('\x0'));

    qDebug() << cmdline.c_str();
    return getAppNameByCmdline(_desktops, cmdline);
}

#if 0
std::string MdmAppEvent::getPyExePath(const uint& _pid)
{
    std::ifstream inFile;
    std::string   line;
    std::string   path = "/proc/" + std::to_string(_pid) +"/cmdline";
    inFile.open(path);
    if (!inFile.is_open()) {
        qDebug() << "MdmAppEvent::getPkgNamePy open file error:" + QString::fromStdString(path);
        return std::string();
    }
    while (!inFile.eof()) {

        if (line.find("python") != std::string::npos) {
            std::string exepath(line.begin() + line.find("python") + 8,
                                line.end());
            // qDebug() << exepath.c_str();
            inFile.close();
            return exepath;
        }
    }
    qDebug() << "proc" + QString(_pid) + "is not a python app";
    inFile.close();
    return std::string();
}
#endif

#if 0
std::string MdmAppEvent::getAppNameByExe(const std::vector<std::string>& _desktops, const std::string& _exe)
{
    // 对照desktopName和从进程目录下exe中取到的可执行文件名，如果一致就反回它
    for (auto begin = _desktops.begin(); begin != _desktops.end(); ++begin) {
        if (*begin == _exe)
             return *begin;
    }

    // 如果不一致就返回一个相似度最高的，这里的相似度找的是包含exe且名称长度最接近的
    unsigned long minSize = 0;
    std::string name;
    for (auto begin = _desktops.begin(); begin != _desktops.end(); ++begin) {
        if (begin->find(_exe) != std::string::npos) {
            if (minSize == 0 || begin->size() < minSize) {
                minSize = begin->size();
                name = *begin;
            }
        }
    }
    if (minSize != 0) {
        qWarning() << QString::fromStdString("getAppNameByExe:This app is run from:" + _exe);
        return name;
    }
    return std::string();
}
#endif

#if 0
std::string MdmAppEvent::getAppUid(const std::string& _fileName)
{
    std::ifstream inFile;
    std::string   line;
    inFile.open(_fileName);
    if(!inFile.is_open()) {
        qDebug() << "Can not open file.";
        return "";
    }
    // 读取UID
    for (int var = 0; var < 9; ++var)
        getline(inFile, line);
    inFile.close();
    uint begin = 4;
    uint end = line.find("\t", begin + 1);
    return line.substr(begin + 1, (end - (begin + 1)));
}
#endif
