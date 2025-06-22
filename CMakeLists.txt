cmake_minimum_required(VERSION 3.5)

project(QtOpencv LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(OpenCV REQUIRED)

set(SOURCES
    main.cpp
    mainwindow.cpp
    videowidget.cpp
    annotationparser.cpp
)

set(HEADERS
    mainwindow.h
    videowidget.h
    annotationparser.h
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets
    ${OpenCV_LIBS}
)
