find_package(PkgConfig REQUIRED)

if (PKG_CONFIG_FOUND)
  pkg_check_modules(cairo REQUIRED cairo)
endif()
