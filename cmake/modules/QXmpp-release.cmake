#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "QXmpp::QXmpp" for configuration "Release"
set_property(TARGET QXmpp::QXmpp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(QXmpp::QXmpp PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${QXMPP_LIBRARIES}"
  )

list(APPEND _IMPORT_CHECK_TARGETS QXmpp::QXmpp )
list(APPEND _IMPORT_CHECK_FILES_FOR_QXmpp::QXmpp "${QXMPP_LIBRARIES}" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
