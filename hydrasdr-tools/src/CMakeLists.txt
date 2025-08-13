# Copyright 2012 Jared Boone
# Copyright 2013-2025 Benjamin Vernoux
#
# This file is part of HydraSDR (based on HackRF project).
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.

if(TARGET HydraSDR::hydrasdr)
  list(APPEND TOOLS_LINK_LIBS HydraSDR::hydrasdr)
else()
  list(APPEND TOOLS_LINK_LIBS HydraSDR::hydrasdr_static)
endif()

if(MSVC)
  add_library(libgetopt_static STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../getopt/getopt.c)
  list(APPEND TOOLS_LINK_LIBS libgetopt_static)
endif()

add_executable(hydrasdr_gpio hydrasdr_gpio.c)
target_link_libraries(hydrasdr_gpio ${TOOLS_LINK_LIBS})
install(TARGETS hydrasdr_gpio RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(hydrasdr_gpiodir hydrasdr_gpiodir.c)
target_link_libraries(hydrasdr_gpiodir ${TOOLS_LINK_LIBS})
install(TARGETS hydrasdr_gpiodir RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(hydrasdr_lib_version hydrasdr_lib_version.c)
target_link_libraries(hydrasdr_lib_version ${TOOLS_LINK_LIBS})
install(TARGETS hydrasdr_lib_version
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(hydrasdr_si5351c hydrasdr_si5351c.c)
target_link_libraries(hydrasdr_si5351c ${TOOLS_LINK_LIBS})
install(TARGETS hydrasdr_si5351c RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(hydrasdr_r82x hydrasdr_r82x.c)
target_link_libraries(hydrasdr_r82x ${TOOLS_LINK_LIBS})
install(TARGETS hydrasdr_r82x RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(hydrasdr_spiflash hydrasdr_spiflash.c)
target_link_libraries(hydrasdr_spiflash ${TOOLS_LINK_LIBS})
install(TARGETS hydrasdr_spiflash RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(hydrasdr_calibrate hydrasdr_calibrate.c)
target_link_libraries(hydrasdr_calibrate ${TOOLS_LINK_LIBS})
install(TARGETS hydrasdr_calibrate RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(hydrasdr_info hydrasdr_info.c)
target_link_libraries(hydrasdr_info ${TOOLS_LINK_LIBS})
install(TARGETS hydrasdr_info RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(hydrasdr_rx hydrasdr_rx.c)
target_link_libraries(hydrasdr_rx ${TOOLS_LINK_LIBS})
install(TARGETS hydrasdr_rx RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(hydrasdr_reset hydrasdr_reset.c)
target_link_libraries(hydrasdr_reset ${TOOLS_LINK_LIBS})
install(TARGETS hydrasdr_reset RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_executable(hydrasdr_set_rf_port hydrasdr_set_rf_port.c)
target_link_libraries(hydrasdr_set_rf_port ${TOOLS_LINK_LIBS})
install(TARGETS hydrasdr_set_rf_port
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
