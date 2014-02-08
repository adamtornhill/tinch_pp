project( tinch_pp )

include(InstallRequiredSystemLibraries)

SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "3")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_PACKAGE_VERSION "0.3.0")

cmake_minimum_required(VERSION 2.8)

enable_testing()

add_subdirectory(impl)
add_subdirectory(tinch_pp)
add_subdirectory(test)

find_package(Doxygen)

if(DOXYGEN_FOUND)
  find_file(DOXYFILE_IN "Doxyfile.in"
    PATHS "${CMAKE_CURRENT_SOURCE_DIR}")
endif(DOXYGEN_FOUND)

mark_as_advanced(DOXYFILE_IN)

if(DOXYGEN_FOUND AND DOXYFILE_IN)
  set(DOXYFILE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/doc" CACHE PATH "Doxygen output directory")
  configure_file(${DOXYFILE_IN} Doxyfile ESCAPE_QUOTES IMMEDIATE @ONLY)

  add_custom_target(doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile WORKING_DIRECTORY ${CMAKE_HOME_DIRECTORY})
else(DOXYGEN_FOUND AND DOXYFILE_IN)
  message("Doxygen not found, unable to build documentation")
endif(DOXYGEN_FOUND AND DOXYFILE_IN)

include(CPack)