# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/joao.santos/esp/v5.1.3/esp-idf/components/bootloader/subproject"
  "C:/Users/joao.santos/esp/TelemetriaGT/build/bootloader"
  "C:/Users/joao.santos/esp/TelemetriaGT/build/bootloader-prefix"
  "C:/Users/joao.santos/esp/TelemetriaGT/build/bootloader-prefix/tmp"
  "C:/Users/joao.santos/esp/TelemetriaGT/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/joao.santos/esp/TelemetriaGT/build/bootloader-prefix/src"
  "C:/Users/joao.santos/esp/TelemetriaGT/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/joao.santos/esp/TelemetriaGT/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/joao.santos/esp/TelemetriaGT/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
