cmake_minimum_required(VERSION 3.12)
project(cppcoro LANGUAGES CXX)

if(NOT "${CMAKE_CXX_STANDARD}")
  message("C++ version not set. Defaulting to CMAKE_CXX_STANDARD=20")
  set(CMAKE_CXX_STANDARD 20)
endif()

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

add_subdirectory(lib)

enable_testing()
if(BUILD_TESTING)
    add_subdirectory(test)
endif()

if(CXX_COROUTINES_HEADER STREQUAL coroutine)
	set(CXX_COROUTINES_COMPONENT Final)
else()
	set(CXX_COROUTINES_COMPONENT Experimental)
endif()

export(EXPORT cppcoroTargets
  FILE "${PROJECT_BINARY_DIR}/cppcoro/cppcoroTargets.cmake"
  NAMESPACE cppcoro::)
configure_file(cmake/cppcoroConfig.cmake
    "${PROJECT_BINARY_DIR}/cppcoro/cppcoroConfig.cmake"
    @ONLY)
configure_file(cmake/FindCoroutines.cmake
    "${PROJECT_BINARY_DIR}/cppcoro/FindCoroutines.cmake"
    COPYONLY)

set(config_package_location lib/cmake/cppcoro)
install(DIRECTORY include/cppcoro
	    DESTINATION include
	    COMPONENT Devel)
install(FILES cmake/FindCoroutines.cmake
	DESTINATION ${config_package_location}
	COMPONENT Devel)
install(EXPORT cppcoroTargets
  FILE cppcoroTargets.cmake
  NAMESPACE cppcoro::
  DESTINATION ${config_package_location})
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/cppcoro/cppcoroConfig.cmake
  DESTINATION ${config_package_location}
  COMPONENT Devel)
