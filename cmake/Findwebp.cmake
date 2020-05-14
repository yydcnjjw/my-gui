find_package(PkgConfig REQUIRED)

if (PKG_CONFIG_FOUND)
  pkg_check_modules(libwebp REQUIRED libwebp)
  pkg_check_modules(libwebpdecoder REQUIRED libwebpdecoder)
  pkg_check_modules(libwebpdemux REQUIRED libwebpdemux)
  pkg_check_modules(libwebpmux REQUIRED libwebpmux)
  set(webp_LIBRARIES
    ${libwebp_LIBRARIES}
    ${libwebpdecoder_LIBRARIES}
    ${libwebpdemux_LIBRARIES}
    ${libwebpmux_LIBRARIES})
endif()
