#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "CNSDK::spdlog" for configuration "RelWithDebInfo"
set_property(TARGET CNSDK::spdlog APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(CNSDK::spdlog PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO "CXX"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/leiaspdlog.lib"
  )

list(APPEND _cmake_import_check_targets CNSDK::spdlog )
list(APPEND _cmake_import_check_files_for_CNSDK::spdlog "${_IMPORT_PREFIX}/lib/leiaspdlog.lib" )

# Import target "CNSDK::leiaCore-faceTrackingInApp-shared" for configuration "RelWithDebInfo"
set_property(TARGET CNSDK::leiaCore-faceTrackingInApp-shared APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(CNSDK::leiaCore-faceTrackingInApp-shared PROPERTIES
  IMPORTED_IMPLIB_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/leiaCore-faceTrackingInApp.lib"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/bin/leiaCore-faceTrackingInApp.dll"
  )

list(APPEND _cmake_import_check_targets CNSDK::leiaCore-faceTrackingInApp-shared )
list(APPEND _cmake_import_check_files_for_CNSDK::leiaCore-faceTrackingInApp-shared "${_IMPORT_PREFIX}/lib/leiaCore-faceTrackingInApp.lib" "${_IMPORT_PREFIX}/bin/leiaCore-faceTrackingInApp.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
