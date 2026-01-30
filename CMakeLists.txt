cmake_minimum_required(VERSION 3.16)

project(deduplikate VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required packages
find_package(ECM 6.0.0 REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

# Qt6 and KDE Frameworks 6
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Concurrent
)

find_package(KF6 REQUIRED COMPONENTS
    CoreAddons
    I18n
    XmlGui
    ConfigWidgets
    WidgetsAddons
    KIO
)

# Build Rust bridge library
add_subdirectory(src/czkawka_bridge)

# Application sources
set(deduplikate_SRCS
    src/main.cpp
    src/mainwindow.cpp
    src/duplicatefinder.cpp
    src/duplicatemodel.cpp
    src/settingsdialog.cpp
)

# Application headers
set(deduplikate_HDRS
    src/mainwindow.h
    src/duplicatefinder.h
    src/duplicatemodel.h
    src/settingsdialog.h
)

# Create the executable
add_executable(deduplikate ${deduplikate_SRCS} ${deduplikate_HDRS})

# Link libraries
target_link_libraries(deduplikate
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
    KF6::CoreAddons
    KF6::I18n
    KF6::XmlGui
    KF6::ConfigWidgets
    KF6::WidgetsAddons
    KF6::KIOCore
    KF6::KIOWidgets
    czkawka_bridge
)

# Include directories
target_include_directories(deduplikate PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/czkawka_bridge
)

# Install
install(TARGETS deduplikate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

# Install desktop file
install(FILES data/deduplikate.desktop DESTINATION ${KDE_INSTALL_APPDIR})

# Install icon
install(FILES data/deduplikate.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
