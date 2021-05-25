# mdm_app_event_service

## 依赖
    debhelper-compat (= 12),
	qt5-qmake,
    qtbase5-dev,
    qtbase5-private-dev,
    qtchooser,
    qtbase5-dev-tools,
    libkf5windowsystem-dev,
    libglib2.0-dev

## build & install

    mkdir build
    cd build
    qmake ..
    make
    sudo make install 

## DBus

    Service: com.mdm.app.event
    Object path: /com/mdm/app/event
    Interface: com.mdm.app.event

## interface

### signal

    ```C++
        /*!
         * \brief send when window open
         * \param appid:application desktop file name
        */
        app_open(UInt32 appid);

        /*!
         * \brief send when window close
         * \param appid:application desktop file name
        */
        app_close(UInt32 appid);

        /*!
         * \brief send when window minimum
         * \param appid:application desktop file name
        */
        app_minimum(String, UInt32)

        /*!
         * \brief send when window get focus
         * \param appid:application desktop file name
        */
        app_get_focus(String, UInt32)

        /*!
         * \brief send when window lose focus
         * \param appid:application desktop file name
        */
        app_lose_focus(String, UInt32)
    ```

### method

    ```C++
        /*!
         * \brief interface of close application
         * \param app_name:application desktop file name
         * \return error number,'0' is succeed,'2' is application is not launch
        */
        uint closeApp(String app_name)
    ```

## test

    log file: /tmp/MdmAppEvent.log
    tail -f /tmp/MdmAppEvent.log
