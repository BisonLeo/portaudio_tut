#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "portaudio" for configuration "Debug"
set_property(TARGET portaudio APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(portaudio PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/portaudio_x86.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/portaudio_x86.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS portaudio )
list(APPEND _IMPORT_CHECK_FILES_FOR_portaudio "${_IMPORT_PREFIX}/lib/portaudio_x86.lib" "${_IMPORT_PREFIX}/bin/portaudio_x86.dll" )

# Import target "portaudio_static" for configuration "Debug"
set_property(TARGET portaudio_static APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(portaudio_static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/portaudio_static_x86.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS portaudio_static )
list(APPEND _IMPORT_CHECK_FILES_FOR_portaudio_static "${_IMPORT_PREFIX}/lib/portaudio_static_x86.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
