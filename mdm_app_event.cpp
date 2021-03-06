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
    if (winData.length()==0)
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


// private function

//WinData MdmAppEvent::getInfoByWid(const WId& _id)
std::string MdmAppEvent::getInfoByWid(const WId& _id)
{
    // 通过KF5提供的KWindowInfo来获取PID
    KWindowInfo wininfo(_id, NET::Property::WMState);
    uint        pid = wininfo.pid();
    std::string name;

    /*!
     * \brief:通过文件流来从文件/proc/pid/status中读取UID和应用名
     * \param:line:按行从文件中读数据，作为行的缓存
     * name:存储窗口所属的应用名称
     * UID:存储用户ID，文本信息取出来是字符串
    */
//    std::string   fileName;
//    std::string   UID;

//    fileName = "/proc/" + std::to_string(pid) + "/status";

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
    /*!
     * \param 进程id
     * \details 首先通过进程号获取启动进程的文件然后查找安装这个文件的安装包名
     * 如果找不到安装包，返回一个错误
     * 查找这个安装包都安装了哪些文件，挑选出其中的*.desktop文件并放入容器
     * 如果一个desktop文件都没找到，返回一个错误
     * 遍历这个容器，如果只安装了一个desktop文件，那就直接返回这个desktop文件名 *。*
     * 如果有多个desktop文件，对比desktop文件的文件名和前面找到的进程启动文件名
     * 如果有一致的则返回那个一致的，如果没有一致的就返回一个最接近的
     * 如果都没有（desktop文件名都没有包含启动文件名的），那么检查desktop文件中的exec
     * 如果exec中有和启动文件名一致的就返回，如果没有就找一个最接近的
     * 如果都没有，那就在这个安装包撞进来的desktop文件中找一个最短的返回出去并带上输出
    */

    char pathBuf[100] = "";
    std::string path = "/proc/" + std::to_string(_pid) +"/exe";
    // qDebug() << path.c_str();
    if (0 != readlink(path.c_str(), pathBuf, 100)) {
        if (strlen(pathBuf) == 0)
            return "";

        std::string pathStr = pathBuf;

        if (pathStr.find("/usr/bin/python") != std::string::npos)
            pathStr = getPkgNamePy(_pid);
        qDebug() << pathStr.c_str();


        std::string pkg = getPkgName(pathStr);
        if (pkg == "") {
            qWarning() << QString::fromStdString(pathStr) + "can not find install packaeg";
            return "";
        }
        std::vector<std::string> desktopName = getPkgContent(pkg);
        if (desktopName.empty()) {
            qWarning() << "MdmAppEvent::getAppName this install package:" + QString::fromStdString(pkg) + "with no desktop file";
            return "";
        }
        if (desktopName.size() == 1) {
            // 如果只有一个desktop文件，万事大吉
            return desktopName[0];
        }

        // 从进程目录的exe中截取启动的文件名
        std::string exe;
        if (pathStr.rfind("/") != std::string::npos) {
            if (pathStr.rfind(" ") != std::string::npos)
                exe = std::string(pathStr.begin()+pathStr.rfind("/")+1,
                                  pathStr.begin()+pathStr.rfind(" ")-1);
            else
                exe = std::string(pathStr.begin()+pathStr.rfind('/')+1,
                                  pathStr.end());
            qDebug() << "exe = " <<exe.c_str();
        }
        else {
            if (pathStr.rfind(" ") != std::string::npos) {
                exe = std::string(pathStr.begin(), pathStr.begin()+pathStr.rfind(" ")-1);
            }
            else
                exe = pathStr;
            qDebug() << "exe = " <<exe.c_str();
        }


        std::string appid = getAppNameByExe(desktopName, exe);
        if (!appid.empty())
            return appid;

        appid = getAppNameByExecPath(desktopName, exe);
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
}

std::string MdmAppEvent::getPkgName(const std::string& _fpath)
{
    std::string comm = "dpkg -S " + _fpath;
    qDebug() << QString::fromStdString(comm);
    FILE*       fp = popen(comm.c_str(), "r");
    char        nameBuf[100] = "";
    fgets(nameBuf, 100, fp);
    pclose(fp);

    std::string pkgInfo = nameBuf;
    if (pkgInfo == "")
        return "";

    return std::string(pkgInfo.begin(),
                       pkgInfo.begin() + pkgInfo.find(':'));
}
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

std::string MdmAppEvent::getWinInfo(const WId& _id)
{
    auto win = m_windowList.find(_id);
    return win->second;
}

std::string MdmAppEvent::getPkgNamePy(const uint& _pid)
{
    std::ifstream inFile;
    std::string   line;
    std::string   path = "/proc/" + std::to_string(_pid) +"/cmdline";
    inFile.open(path);
    if (!inFile.is_open()) {
        qDebug() << "MdmAppEvent::getPkgNamePy open file error:" + QString::fromStdString(path);
        return "";
    }
    while (!inFile.eof()) {
        getline(inFile, line);
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
    return "";
}

std::vector<std::string> MdmAppEvent::getPkgContent(const std::string& _pkgname)
{
    std::string comm = "dpkg -L " + _pkgname;
    FILE*       fp = popen(comm.c_str(), "r");
    char        buff[255];
    if (fp == NULL) {
        qWarning() << "getPkgContent: dpkg -L error";
        return std::vector<std::string>();
    }
    std::vector<std::string> desktopName;
    for (int i = 0; fgets(buff, 255, fp) != NULL; ++i) {
        // qDebug() << buff;
        std::string line = buff;
        if (line.find(".desktop") != std::string::npos) {
            desktopName.push_back(std::string(line.begin()+line.rfind("/")+1,
                                              line.begin()+line.rfind(".desktop")));
        }
    }
    pclose(fp);
    return desktopName;
}

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
    return "";
}

std::string MdmAppEvent::getAppNameByExecPath(const std::vector<std::string>& _desktops, const std::string& _exe)
{
    // 如果没有找到包含exe的desktop名，则用可执行文件名exe和desktop文件中exec的路径名对比，看看exec路径中有没有包含的
    std::vector<std::pair<std::string, std::string>> execInDesktop;
    for (auto begin = _desktops.begin(); begin != _desktops.end(); ++begin) {
        std::ifstream inFile;
        std::string   line;
        std::string   exec;

        inFile.open("/usr/share/applications/" + *begin + ".desktop");
        // qDebug() << QString::fromStdString("/usr/share/applications/" + *begin + ".desktop");
        if (!inFile.is_open()) {
            qWarning() << QString::fromStdString("MdmAppEvent::getAppName(const uint& _pid) Open File Error:/usr/share/applications/" + *begin + ".desktop");
            continue;
        }
        // 遍历desktop文件，将其中相关的exec存储到容器中
        while (getline(inFile, line)) {
            if (line.find("Exec") == std::string::npos)
                continue;

            if (line.rfind("/") != std::string::npos) {
                if (line.rfind(" ") != std::string::npos)
                    exec = std::string(line.begin()+line.rfind("/")+1, line.begin()+line.rfind(" "));
                else
                    exec = std::string(line.begin()+line.rfind("/")+1, line.end());
            }
            else {
                if (line.rfind(" ") != std::string::npos)
                    exec = std::string(line.begin(), line.begin() + line.rfind(" "));
                else
                    exec = std::string(line.begin(), line.end());
            }
            // qDebug() << line.c_str();
            if (exec.find(_exe) != std::string::npos) {
                // 全都放到外部容器中，容器中是 exec:desktop
                execInDesktop.push_back(std::make_pair(exec, *begin));
                // return *begin;
            }
        }
        inFile.close();
    }
    // 从execInDesktop找一个最相近的
    unsigned long minSize = 0;
    std::string name;
    for (auto begin = execInDesktop.begin(); begin != execInDesktop.end(); ++begin) {
        if (minSize == 0 || begin->first.size() < minSize) {
            minSize = begin->first.size();
            name = begin->second;
        }
    }
    if (minSize != 0)
        return name;
    return "";
}

std::string MdmAppEvent::getAppNameByTxappid(const std::string _tx_appip)
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
        return "";
    }
}
