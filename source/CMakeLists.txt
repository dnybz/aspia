#
# Aspia Project
# Copyright (C) 2020 Dmitry Chapyshev <dmitry@aspia.ru>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.
#

include(CheckIPOSupported)

# Qt configuration.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
list(APPEND CMAKE_AUTORCC_OPTIONS -compress 9 -threshold 5)

if (WIN32)
    # Target version.
    add_definitions(-DNTDDI_VERSION=0x06010000
                    -D_WIN32_WINNT=0x0601
                    -D_WIN32_WINDOWS=_WIN32_WINNT
                    -DWINVER=_WIN32_WINNT
                    -D_WIN32_IE=0x0800
                    -DPSAPI_VERSION=2)

    # Other definitions.
    add_definitions(-D_UNICODE
                    -DUNICODE
                    -DWIN32_LEAN_AND_MEAN
                    -DNOMINMAX)
endif()

# For Asio.
add_definitions(-DASIO_STANDALONE
                -DASIO_NO_DEPRECATED
                -D_SILENCE_CXX17_ALLOCATOR_VOID_DEPRECATION_WARNING)

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "AMD64" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86")
    # For RapidJSON.
    add_definitions(-DRAPIDJSON_SSE2)
endif()

# For Qt.
add_definitions(-DQT_NO_CAST_TO_ASCII
                -DQT_NO_CAST_FROM_BYTEARRAY
                -DQT_USE_QSTRINGBUILDER)

set(QT_COMMON_LIBS
    Qt5::Core
    Qt5::CorePrivate
    Qt5::QGenericEnginePlugin
    Qt5::Gui
    Qt5::GuiPrivate
    Qt5::QICOPlugin
    Qt5::Network
    Qt5::NetworkPrivate
    Qt5::PrintSupport
    Qt5::PrintSupportPrivate
    Qt5::Widgets
    Qt5::WidgetsPrivate
    Qt5::Xml
    Qt5::XmlPrivate)

if (WIN32)
    set(QT_PLATFORM_LIBS
        Qt5::WinMain
        Qt5::WinExtras
        Qt5::WinExtrasPrivate
        Qt5::QWindowsIntegrationPlugin
        Qt5::QWindowsVistaStylePlugin
        Qt5::QWindowsPrinterSupportPlugin)
endif()

set(THIRD_PARTY_LIBS
    protobuf::libprotobuf-lite
    zstd::libzstd_static
    OpenSSL::Crypto
    OpenSSL::SSL
    Opus::opus
    modp_b64
    unofficial::libvpx::libvpx
    x11region
    yuv)

include_directories(${PROJECT_SOURCE_DIR}/source ${PROJECT_BINARY_DIR}/source)

# C++ compliller flags.
set(CMAKE_CXX_STANDARD 17)

if (MSVC)
    # C++ compliller flags.
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /W3 /MP /arch:SSE2")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MP /W3")

    # C compiller flags.
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /W3 /MP /arch:SSE2")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /W3 /MP")

    # Static runtime library.
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT error)

if (IPO_SUPPORTED)
    message(STATUS "IPO/LTO supported")

    # Enable link-time code generatation.
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO/LTO not supported")
endif()

# Compiller flags.
message(STATUS "CXX flags (release): ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "CXX flags (debug): ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "C flags (release): ${CMAKE_C_FLAGS_RELEASE}")
message(STATUS "C flags (debug): ${CMAKE_C_FLAGS_DEBUG}")

# Linker flags.
message(STATUS "Linker flags (shared release): ${CMAKE_SHARED_LINKER_FLAGS_RELEASE}")
message(STATUS "Linker flags (shared debug): ${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
message(STATUS "Linker flags (static release): ${CMAKE_STATIC_LINKER_FLAGS_RELEASE}")
message(STATUS "Linker flags (static debug): ${CMAKE_STATIC_LINKER_FLAGS_DEBUG}")
message(STATUS "Linker flags (exe release): ${CMAKE_EXE_LINKER_FLAGS_RELEASE}")
message(STATUS "Linker flags (exe debug): ${CMAKE_EXE_LINKER_FLAGS_DEBUG}")

add_subdirectory(base)
add_subdirectory(client)
add_subdirectory(common)
add_subdirectory(console)
add_subdirectory(proto)
add_subdirectory(qt_base)
add_subdirectory(relay)
add_subdirectory(router)
add_subdirectory(third_party)

if (WIN32)
    add_subdirectory(host)
endif()
