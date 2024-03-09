cmake_minimum_required(VERSION 3.5)

#
set(PROJECTNAME "pathTracerNRC")
PROJECT(${PROJECTNAME})

# 
file(GLOB SOURCE_FILES *.cpp *.hpp *.inl *.h *.c)

#
set(NVPROCORE nvpro_core/cmake/setup.cmake)
unset(BASE_DIRECTORY CACHE)
if(NOT BASE_DIRECTORY)
    find_path(BASE_DIRECTORY
              NAMES ${NVPROCORE}
              PATHS ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../..
              REQUIRED
              DOC "Directory containing nvpro_core")
endif()

if(EXISTS ${BASE_DIRECTORY}/${NVPROCORE})
    include(${BASE_DIRECTORY}/${NVPROCORE})
else()
    message(FATAL_ERROR "could not find base directory, please set BASE_DIRECTORY to folder containing nvpro_core")
endif()


# funtion in nvpro_core: load package 
_add_package_VulkanSDK()
_add_nvpro_core_lib()

# 
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/src/")
# add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/ext/nvpro_core/")