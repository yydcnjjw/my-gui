find_package(PkgConfig REQUIRED)

if (PKG_CONFIG_FOUND)
  pkg_check_modules(SDL2Mixer REQUIRED SDL2_mixer)
endif()
