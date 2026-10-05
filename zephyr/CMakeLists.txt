# SPDX-License-Identifier: MIT

if(CONFIG_LIBVRT)
  set(LIBVRT_DIR ${ZEPHYR_CURRENT_MODULE_DIR})

  zephyr_include_directories(${LIBVRT_DIR}/include)

  zephyr_library()
  zephyr_library_sources(
    ${LIBVRT_DIR}/src/timestamp_to_calendar.c
    ${LIBVRT_DIR}/src/vrt_fixed_point.c
    ${LIBVRT_DIR}/src/vrt_init.c
    ${LIBVRT_DIR}/src/vrt_read.c
    ${LIBVRT_DIR}/src/vrt_string.c
    ${LIBVRT_DIR}/src/vrt_time.c
    ${LIBVRT_DIR}/src/vrt_util.c
    ${LIBVRT_DIR}/src/vrt_util_internal.c
    ${LIBVRT_DIR}/src/vrt_words.c
    ${LIBVRT_DIR}/src/vrt_write.c
  )
  zephyr_library_include_directories(${LIBVRT_DIR}/src)
endif()
