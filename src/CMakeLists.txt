set(PROJECT quepanel)

set(SOURCES
    actionview.cpp
    plugins/clock/clocklabel.cpp
    main.cpp
    plugins/mainmenu/mainmenu.cpp
    mainpanel.cpp
    resources.cpp
    plugins/taskbar/qmtaskgroup.cpp
    plugins/taskbar/qmtaskbar.cpp
    plugins/systray/systray.cpp
    plugins/systray/trayicon.cpp
)

set(LIBRARIES
    ${CMAKE_THREAD_LIBS_INIT}
    KF5::WindowSystem
    ${X11_LIBRARIES}
    ${XCB_LIBRARIES}
    ${GIO_LIBRARIES}
    ${QT_LIBRARIES}
)

include_directories(
    ${GIO_INCLUDE_DIRS}
    ${QT_INCLUDE_DIRS}
)

project(${PROJECT})
add_executable(${PROJECT} ${SOURCES})
target_link_libraries(${PROJECT} ${LIBRARIES})
install(TARGETS ${PROJECT} RUNTIME DESTINATION bin)
