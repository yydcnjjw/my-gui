find_package(PkgConfig REQUIRED)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(FFMPEG_AVCODEC libavcodec)
  pkg_check_modules(FFMPEG_AVFORMAT libavformat)
  pkg_check_modules(FFMPEG_AVUTIL libavutil)

  set(FFMPEG_LIBRARIES
    ${FFMPEG_AVCODEC_LIBRARIES}
    ${FFMPEG_AVFORMAT_LIBRARIES}
    ${FFMPEG_AVUTIL_LIBRARIES})
endif()
