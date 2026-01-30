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
    Config
)

# Build Rust bridge library
add_subdirectory(src/czkawka_bridge)

# Core library sources (without main.cpp for testing)
set(deduplikate_core_SRCS
    src/mainwindow.cpp
    src/duplicatefinder.cpp
    src/duplicatemodel.cpp
    src/settingsdialog.cpp
)

set(deduplikate_core_HDRS
    src/mainwindow.h
    src/duplicatefinder.h
    src/duplicatemodel.h
    src/settingsdialog.h
)

# Create a static library for core functionality (used by tests)
add_library(deduplikate_core STATIC ${deduplikate_core_SRCS} ${deduplikate_core_HDRS})

target_link_libraries(deduplikate_core PUBLIC
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
    KF6::ConfigCore
    czkawka_bridge
)

target_include_directories(deduplikate_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/czkawka_bridge
)

# Create the executable
add_executable(deduplikate src/main.cpp)

# Link against core library
target_link_libraries(deduplikate PRIVATE deduplikate_core)

# Install
install(TARGETS deduplikate ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

# Install desktop file
install(FILES data/deduplikate.desktop DESTINATION ${KDE_INSTALL_APPDIR})

# Install icon
install(FILES data/deduplikate.svg DESTINATION ${KDE_INSTALL_FULL_ICONDIR}/hicolor/scalable/apps)

# Testing
option(BUILD_TESTING "Build the testing tree" ON)
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
