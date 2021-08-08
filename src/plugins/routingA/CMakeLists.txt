set(CMAKE_INCLUDE_CURRENT_DIR ON)

qv2ray_add_plugin(QvPlugin-RoutingAEditor Widgets
    INSTALL_PREFIX_MACOS "$<TARGET_BUNDLE_DIR:qv2ray>/Contents/Resources/plugins"
    CLASS_NAME "RoutingAPlugin"
    DEV_INTERFACE)

target_sources(QvPlugin-RoutingAEditor PRIVATE
    RoutingAPlugin.cpp
    RoutingAPlugin.hpp
    RoutingA_GuiInterface.hpp
    core/libRoutingA-ActionTable.cpp
    core/libRoutingA.cpp
    core/libRoutingA.hpp
    common/RoutingAConverter.cpp
    common/RoutingAConverter.hpp
    ui/RoutingAEditor.cpp
    ui/RoutingAEditor.ui
    ui/RoutingAEditor.hpp
    )

target_include_directories(QvPlugin-RoutingAEditor PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(QvPlugin-RoutingAEditor PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../PluginsCommon)
