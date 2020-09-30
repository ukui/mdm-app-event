# mdm_app_event_service

## changelog
	* 将输入和输出改为/usr/share/applichations/下.desktop文件名
	* 增加开机自启功能，安装后下一次开机会自动启动在后台，可以通过d-feet查看
	* 服务崩溃后会自动拉起
	* 安装位置改为/usr/bin/MdmAppEvent，可以通过执行MdmAppEvent直接运行
	* 修复打开搜索不到安装包的应用时崩溃的bug
	* 修复只关闭相同应用的一个窗口bug

## 依赖
    sudo apt install libkf5windowsystem-dev

## 安装
    dpkg -i *.deb
    会默认安装到/opt/MdmAppEvent/bin目录下

## 信号
    app_open(String, UInt32)
        窗口打开时发出

    app_close(String, UInt32)
        窗口关闭时发出

    app_minimum(String, UInt32)
        窗口最小化时发出

    app_get_focus(String, UInt32)
        窗口得到焦点时发出

    app_lose_focus(String, UInt32)
        窗口失去焦点时发出

所有信号中的String都是代表发出信号的窗口所属的应用的应用的.desktop文件名，如chrome对应google-chrome.desktop，就输出google-chrome
所有信号中的UInt32都是打开窗口的用户的UID

## 接口

    closeApp(String app_name, UInt32 userid)->(int32 err_num)
    传入要关闭窗口所属的应用包名和UID，关闭相应的窗口
    返回值为0，代表关闭App成功
    返回值为1，代表参数错误。userid不能为0，0为root⽤⼾，userid应该⼤于等于1000
    返回值为2，代表该程序未运⾏
    返回值为3，代表未知错误

## 测试
    x86本地机+优麒麟20.04环境  
    反复打开关闭切换qtcreator、qt助手、终端、Chrome、Firefox、vscode、peony等
    服务正常工作


