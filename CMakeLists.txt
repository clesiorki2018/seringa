# Copyright 2026 clesiorki
#
# Licensed under the Apache License, Version 2.0.
# See the LICENSE file in the project root for full license information.

# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

set(SERINGA_ENV_FILE "${CMAKE_SOURCE_DIR}/.env")

if(EXISTS "${SERINGA_ENV_FILE}")
    file(STRINGS "${SERINGA_ENV_FILE}" SERINGA_ENV_LINES)

    foreach(SERINGA_ENV_LINE IN LISTS SERINGA_ENV_LINES)
        string(STRIP "${SERINGA_ENV_LINE}" SERINGA_ENV_LINE)

        if(SERINGA_ENV_LINE STREQUAL "" OR SERINGA_ENV_LINE MATCHES "^#")
            continue()
        endif()

        if(SERINGA_ENV_LINE MATCHES "^([A-Za-z_][A-Za-z0-9_]*)=(.*)$")
            set(SERINGA_ENV_KEY "${CMAKE_MATCH_1}")
            set(SERINGA_ENV_VALUE "${CMAKE_MATCH_2}")

            string(REGEX REPLACE "^\"(.*)\"$" "\\1" SERINGA_ENV_VALUE "${SERINGA_ENV_VALUE}")
            string(REGEX REPLACE "^'(.*)'$" "\\1" SERINGA_ENV_VALUE "${SERINGA_ENV_VALUE}")

            if(SERINGA_ENV_KEY MATCHES "^SERINGA_")
                set("${SERINGA_ENV_KEY}" "${SERINGA_ENV_VALUE}")
            endif()
        endif()
    endforeach()
endif()

foreach(SERINGA_REQUIRED_VAR
        SERINGA_WIFI_SSID
        SERINGA_WIFI_PASS
        SERINGA_PIN_HASH)
    if(NOT DEFINED "${SERINGA_REQUIRED_VAR}" AND DEFINED ENV{${SERINGA_REQUIRED_VAR}})
        set("${SERINGA_REQUIRED_VAR}" "$ENV{${SERINGA_REQUIRED_VAR}}")
    endif()

    if(NOT DEFINED "${SERINGA_REQUIRED_VAR}" OR "${${SERINGA_REQUIRED_VAR}}" STREQUAL "")
        message(FATAL_ERROR
            "${SERINGA_REQUIRED_VAR} nao configurado. Crie .env a partir de .env.example.")
    endif()
endforeach()

add_compile_definitions(
    "SERINGA_WIFI_SSID=\"${SERINGA_WIFI_SSID}\""
    "SERINGA_WIFI_PASS=\"${SERINGA_WIFI_PASS}\""
    "SERINGA_PIN_HASH=${SERINGA_PIN_HASH}"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(seringa)
spiffs_create_partition_image(spiffs data FLASH_IN_PROJECT)
