cmake_minimum_required(VERSION 2.8)
project( DAMICMCtrl )

#FIND_PACKAGE(Boost REQUIRED COMPONENTS system)
#IF(Boost_FOUND)
#  INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
#  LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})
#ENDIF(Boost_FOUND)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} .)
find_package(ROOT)




set(INCLUDE_DIRECTORIES
${ROOT_INCLUDE_DIR}
#${CFITSIO_INCLUDE_DIR}
)
include_directories(${ROOT_INCLUDE_DIR})


set(LINK_DIRECTORIES
${ROOT_LIBRARY_DIR}
)
link_directories( ${LINK_DIRECTORIES})


set (CMAKE_CXX_FLAGS "-Wno-write-strings -std=gnu++1y -I$ENV{ROOTSYS}/include -O3 -march=native -m64")
set (CMAKE_EXE_LINKER_FLAGS "-Wno-write-strings -I$ENV{ROOTSYS}/include -L$ENV{ROOTSYS}/lib -lGui -lCore -lRIO -lNet -lHist -lGraf -lGraf3d -lGpad -lTree -lRint -lTreePlayer -lPostscript -lMatrix -lPhysics -lMathCore -lThread -pthread -rdynamic -lpthread -lz -lm -ldl")




set(SOURCE
   ${CMAKE_CURRENT_SOURCE_DIR}/SerialDeviceT.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/LakeShoreHeater.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/Cryocooler.cpp
   #${CMAKE_CURRENT_SOURCE_DIR}/helpers.cpp
)
set(HEADERS
   ${CMAKE_CURRENT_SOURCE_DIR}/SerialDeviceT.hpp
   ${CMAKE_CURRENT_SOURCE_DIR}/LakeShoreHeater.hpp
   ${CMAKE_CURRENT_SOURCE_DIR}/Cryocooler.hpp
)




#add_executable( DAMICMCtrl DAMICMTest.cpp )
#target_link_libraries( DAMICMCtrl ${SOURCE} ${HEADERS})


add_executable( DAMICRTest DAMICMRawTest.cpp )
target_link_libraries( DAMICRTest ${SOURCE} ${HEADERS})
#
