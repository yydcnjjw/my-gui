find_package(PkgConfig REQUIRED)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(FFMPEG_AVCODEC libavcodec)
  pkg_check_modules(FFMPEG_AVFORMAT libavformat)
  pkg_check_modules(FFMPEG_AVUTIL libavutil)
  pkg_check_modules(FFMPEG_SWSCALE libswscale)
  pkg_check_modules(FFMPEG_SWRESAMPLE libswresample)

  set(FFMPEG_LIBRARIES
    ${FFMPEG_AVCODEC_LIBRARIES}
    ${FFMPEG_AVFORMAT_LIBRARIES}
    ${FFMPEG_AVUTIL_LIBRARIES}
    ${FFMPEG_SWSCALE_LIBRARIES}
    ${FFMPEG_SWRESAMPLE_LIBRARIES})
endif()
