cmake_minimum_required(VERSION 3.5)

# 
get_filename_component(PROJNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(PROJNAME "${PROJNAME}_PATHTRACINNG")
PROJECT(${PROJNAME})

# project definition for nvpro
_add_project_definitions(${PROJNAME})


# source files
file(GLOB SOURCE_FILES *.cpp *.hpp *.inl *.h *.c)


# shader files
SET(VULKAN_TARGET_ENV vulkan1.2)
UNSET(GLSL_SOURCES)
UNSET(SPV_OUTPUT)

file(GLOB_RECURSE GLSL_HEADER_FILES "shaders/*.h")
file(GLOB_RECURSE GLSL_SOURCE_FILES "shaders/*.glsl")
foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    # shader compilation function from nvpro_core
    _compile_GLSL(${GLSL} "shaders/${FILE_NAME}.spv" GLSL_SOURCES SPV_OUTPUT)
endforeach(GLSL)

list(APPEND GLSL_SOURCES ${GLSL_HEADER_FILES})

#
add_executable(${PROJNAME} ${SOURCE_FILES} ${COMMON_SOURCE_FILES} ${GLSL_SOURCES})

# link nvpro_core library
target_link_libraries(${PROJNAME} ${PLATFORM_LIBRARIES} nvpro_core)

#
foreach(DEBUGLIB ${LIBRARIES_DEBUG})
  target_link_libraries(${PROJNAME} debug ${DEBUGLIB})
endforeach(DEBUGLIB)

foreach(RELEASELIB ${LIBRARIES_OPTIMIZED})
  target_link_libraries(${PROJNAME} optimized ${RELEASELIB})
endforeach(RELEASELIB)

#
_finalize_target( ${PROJNAME} )

#
install(FILES ${SPV_OUTPUT} CONFIGURATIONS Release DESTINATION "bin_${ARCH}/${PROJNAME}/shaders")
install(FILES ${SPV_OUTPUT} CONFIGURATIONS Debug DESTINATION "bin_${ARCH}_debug/${PROJNAME}/shaders")
install(DIRECTORY "./scenes" CONFIGURATIONS Release DESTINATION "bin_${ARCH}/${PROJNAME}")
install(DIRECTORY "./scenes" CONFIGURATIONS Debug DESTINATION "bin_${ARCH}_debug/${PROJNAME}")