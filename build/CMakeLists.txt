cmake_minimum_required(VERSION 3.16)
project(main)

set(CMAKE_CXX_STANDARD 17)

#find_package(Qt6 REQUIRED COMPONENTS Widgets)
find_package( OpenCV REQUIRED )
find_package(wxWidgets REQUIRED)

include_directories( ${OpenCV_INCLUDE_DIRS})

include(UsewxWidgets)



add_executable(main main.cpp cApp.cpp cMain.cpp server.cpp config.cpp login.cpp camera.cpp)
#target_link_libraries(main Qt6::Widgets)
target_link_libraries( main ${OpenCV_LIBS} ${wxWidgets_LIBRARIES})
