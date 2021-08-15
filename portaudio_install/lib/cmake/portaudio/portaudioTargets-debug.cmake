#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "PortAudio::PortAudio" for configuration "Debug"
set_property(TARGET PortAudio::PortAudio APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(PortAudio::PortAudio PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/lib/portaudio.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/bin/portaudio.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS PortAudio::PortAudio )
list(APPEND _IMPORT_CHECK_FILES_FOR_PortAudio::PortAudio "${_IMPORT_PREFIX}/lib/portaudio.lib" "${_IMPORT_PREFIX}/bin/portaudio.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
