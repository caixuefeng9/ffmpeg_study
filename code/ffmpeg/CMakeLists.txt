cmake_minimum_required(VERSION 3.0.0)
project(ffmpeg VERSION 0.1.0)

include(CTest)
enable_testing()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

include_directories(lib/ffmpeg/include)

link_directories(${PROJECT_SOURCE_DIR}/lib/ffmpeg/lib)
link_directories("D:/Software/mingw64/x86_64-w64-mingw32/lib")
#find_library(FFMPEG_LIB libavcodec.dll.a ${PROJECT_SOURCE_DIR}/lib/ffmpeg/lib NO_DEFAULT_PATH)
#MESSAGE( FATAL_ERROR "Thsi is " ${FFMPEG_LIB})

add_executable(ffmpeg main.cpp)

#TARGET_LINK_LIBRARIES(ffmpeg ${FFMPEG_LIB})
TARGET_LINK_LIBRARIES(ffmpeg libavformat.dll.a)
TARGET_LINK_LIBRARIES(ffmpeg libavcodec.dll.a)
TARGET_LINK_LIBRARIES(ffmpeg libavdevice.dll.a)
TARGET_LINK_LIBRARIES(ffmpeg libavfilter.dll.a)
TARGET_LINK_LIBRARIES(ffmpeg libavutil.dll.a)
TARGET_LINK_LIBRARIES(ffmpeg libpostproc.dll.a)
TARGET_LINK_LIBRARIES(ffmpeg libswresample.dll.a)
TARGET_LINK_LIBRARIES(ffmpeg libswscale.dll.a)
TARGET_LINK_LIBRARIES(ffmpeg libwinmm.a)

