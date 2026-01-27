cmake_minimum_required(VERSION 3.16)

project(ogp_changer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Widgets QUIET)
if(NOT Qt6Widgets_FOUND)
    message(FATAL_ERROR
			"\nQt6 Widgets not found. Please install the Qt6 development package.\n"
			"Debian / Ubuntu:\n" 
			"	sudo apt install qt6-base-dev\n"
			"Fedora:\n" 
			"	sudo dnf install qt6-qtbase-devel\n"
			"Arch:\n" 
			"	sudo pacman -S qt6-base\n\n"
			"NOTE: The exact package names might vary or change over time " 
			"depending on the distribution. In case of problems, please check "
			"what naming your distribution uses.")
endif()
find_package(KF6CoreAddons QUIET)
if(NOT KF6CoreAddons_FOUND)
	message(FATAL_ERROR
			"\nKF6 CoreAddons not found. Please install KDE Frameworks 6 CoreAddons development package.\n"
			"Debian / Ubuntu:\n" 
			"	sudo apt install libkf6coreaddons-dev\n"
			"Fedora:\n" 
			"	sudo dnf install kf6-kcoreaddons-devel\n"
			"Arch:\n" 
			"	sudo pacman -S kcoreaddons\n\n"
			"NOTE: The exact package names might vary or change over time " 
			"depending on the distribution. In case of problems, please check "
			"what naming your distribution uses.")
endif()
find_package(KF6KIO QUIET)
if(NOT KF6KIO_FOUND)
	message(FATAL_ERROR
			"\nKF6 KIO not found. Please install KDE Frameworks 6 KIO development package.\n"
			"Debian / Ubuntu:\n" 
			"	sudo apt install libkf6kio-dev\n"
			"Fedora:\n" 
			"	sudo dnf install kf6-kio-devel\n"
			"Arch:\n" 
			"	sudo pacman -S kio\n\n"
			"NOTE: The exact package names might vary or change over time " 
			"depending on the distribution. In case of problems, please check "
			"what naming your distribution uses.")
endif()

# ==================== build cogp ==============================
add_executable(cogp cogp.cpp)
# ==============================================================

# ================ build ogp_changer ===========================
add_executable(ogp_changer ogp_changer.cpp)
target_link_libraries(ogp_changer PRIVATE Qt6::Widgets)
# ==============================================================

# ================ build ogp_changer_plugin ===========================
add_library(ogp_changer_fileitemaction MODULE ogp_changer_plugin.cpp)
set_target_properties(ogp_changer_fileitemaction PROPERTIES PREFIX "")
target_link_libraries(ogp_changer_fileitemaction PRIVATE
					  Qt6::Widgets
					  KF6::KIOWidgets
					  KF6::KIOCore
					  KF6::CoreAddons)
# =====================================================================
