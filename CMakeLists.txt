cmake_minimum_required(VERSION 2.8)
project( DAMICMCtrl )



set (CMAKE_CXX_FLAGS "-Wno-write-strings -std=gnu++1y -I/usr/include/mysql-cppconn-8 -O3 -march=native -m64")
set (CMAKE_EXE_LINKER_FLAGS "-Wno-write-strings -lmysqlcppconn8 -lm")


set(SOURCECC
   ${CMAKE_CURRENT_SOURCE_DIR}/SerialDeviceT.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/Cryocooler.cpp
   #${CMAKE_CURRENT_SOURCE_DIR}/helpers.cpp
)
set(HEADERSCC
   ${CMAKE_CURRENT_SOURCE_DIR}/SerialDeviceT.hpp
   ${CMAKE_CURRENT_SOURCE_DIR}/Cryocooler.hpp
   ${CMAKE_CURRENT_SOURCE_DIR}/MysqlCredentials.hpp
)


set(SOURCELSH
   ${CMAKE_CURRENT_SOURCE_DIR}/SerialDeviceT.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/LakeShoreHeater.cpp
   #${CMAKE_CURRENT_SOURCE_DIR}/helpers.cpp
)
set(HEADERSLSH
   ${CMAKE_CURRENT_SOURCE_DIR}/SerialDeviceT.hpp
   ${CMAKE_CURRENT_SOURCE_DIR}/LakeShoreHeater.hpp
   ${CMAKE_CURRENT_SOURCE_DIR}/MysqlCredentials.hpp
)

set(SOURCESM
   ${CMAKE_CURRENT_SOURCE_DIR}/CryoControlSM.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/PID_v1.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/CCMySQLInteraction.cpp
)
set(HEADERSSM
   ${CMAKE_CURRENT_SOURCE_DIR}/PID_v1.h
   ${CMAKE_CURRENT_SOURCE_DIR}/CryoControlSM.hpp
   ${CMAKE_CURRENT_SOURCE_DIR}/MysqlCredentials.hpp
)

#add_executable( DAMICMCtrl DAMICMTest.cpp )
#target_link_libraries( DAMICMCtrl ${SOURCE} ${HEADERS})


#add_executable( DAMICRTest DAMICMRawTest.cpp )
#target_link_libraries( DAMICRTest ${SOURCE} ${HEADERS})

#add_executable( CCDrone CCDroneStart.cpp )
#target_link_libraries( CCDrone ${SOURCECC} ${HEADERSCC})

#add_executable( LSHDrone LSHDroneStart.cpp )
#target_link_libraries( LSHDrone ${SOURCELSH} ${HEADERSLSH})



add_executable( StartSM StartSM.cpp )
target_link_libraries( StartSM ${SOURCESM} ${HEADERSSM})
#
