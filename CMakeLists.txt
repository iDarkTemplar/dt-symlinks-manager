cmake_minimum_required( VERSION 3.14 )

project(DT-Symlinks-Manager
	VERSION 1.0.2
	LANGUAGES CXX)

include(GNUInstallDirs)

set (CMAKE_CXX_STANDARD 17)

add_definitions(-DCONFIG_DIR=\"${CMAKE_INSTALL_FULL_SYSCONFDIR}\")
add_definitions(-D_FILE_OFFSET_BITS=64)

set ( SOURCES main.cpp input-reader.cpp )
set ( HEADERS          input-reader.hpp )

add_executable( dt-symlinks-manager ${SOURCES} ${HEADERS})
target_link_libraries( dt-symlinks-manager )

# installation config
install(TARGETS dt-symlinks-manager RUNTIME DESTINATION ${CMAKE_INSTALL_SBINDIR})
install(FILES "dt-symlinks-manager.conf"    DESTINATION "${CMAKE_INSTALL_FULL_SYSCONFDIR}")
