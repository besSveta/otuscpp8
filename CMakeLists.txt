cmake_minimum_required(VERSION 3.5)

project(bayan VERSION 0.0.$ENV{TRAVIS_BUILD_NUMBER})
find_package(Threads REQUIRED)
find_package(Boost COMPONENTS unit_test_framework program_options filesystem regex system REQUIRED)

set(Boost_USE_STATIC_LIBS ON)
include_directories(${Boost_INCLUDE_DIR}) 

add_executable(bayan bayan.cpp)
add_library(lib bayan_lib.cpp)
add_executable(bayan_test bayan_test.cpp)

set_target_properties(bayan lib bayan_test PROPERTIES
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	COMPILE_OPTIONS "-Wpedantic;-Wall;-Wextra"
)

set_target_properties(bayan_test PROPERTIES
    COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK
	COMPILE_DEFINITIONS BOOST_ALL_DYN_LINK
)
set_target_properties(bayan PROPERTIES
    COMPILE_DEFINITIONS BOOST_ALL_DYN_LINK
)
target_link_libraries(bayan 
	${Boost_LIBRARIES}
	lib
    Threads::Threads
)

target_link_libraries(bayan_test	
	${Boost_LIBRARIES}
	lib
)
	
install(TARGETS bayan RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)

set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")


set(CPACK_PACKAGE_CONTACT bessveta@gmail.com)

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

include(CPack)

enable_testing()

add_test(bayanTest bayan_test)
