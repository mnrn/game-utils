# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# - Try to find libuv
# Once done, this will define
#
#  LIBUV_FOUND - system has libuv
#  LIBUV_INCLUDE_DIRS - the libuv include directories
#  LIBUV_LIBRARIES - link these to use libuv
#
# https://github.com/neovim/neovim/blob/master/cmake/FindLibUV.cmake
#

find_path(LIBUV_INCLUDE_DIR uv.h
  HINTS ${LIBUV_INCLUDEDIR} ${LIBUV_INCLUDE_DIRS})

list(APPEND LIBUV_NAMES uv)

find_library(LIBUV_LIBRARY NAMES ${LIBUV_NAMES}
  HINTS ${LIBUV_LIBDIR} ${LIBUV_LIBRARY_DIRS})

mark_as_advanced(LIBUV_INCLUDE_DIR LIBUV_LIBRARY)

set(LIBUV_LIBRARIES ${LIBUV_LIBRARY} ${LIBUV_LIBRARIES})
set(LIBUV_INCLUDE_DIRS ${LIBUV_INCLUDE_DIR})

# Deal with the fact that libuv.pc is missing important dependency information.

include(CheckLibraryExists)

check_library_exists(dl dlopen "dlfcn.h" HAVE_LIBDL)
if(HAVE_LIBDL)
  list(APPEND LIBUV_LIBRARIES dl)
endif()

check_library_exists(kstat kstat_lookup "kstat.h" HAVE_LIBKSTAT)
if(HAVE_LIBKSTAT)
  list(APPEND LIBUV_LIBRARIES kstat)
endif()

check_library_exists(kvm kvm_open "kvm.h" HAVE_LIBKVM)
if(HAVE_LIBKVM AND NOT CMAKE_SYSTEM_NAME STREQUAL "OpenBSD")
  list(APPEND LIBUV_LIBRARIES kvm)
endif()

check_library_exists(nsl gethostbyname "nsl.h" HAVE_LIBNSL)
if(HAVE_LIBNSL)
  list(APPEND LIBUV_LIBRARIES nsl)
endif()

check_library_exists(perfstat perfstat_cpu "libperfstat.h" HAVE_LIBPERFSTAT)
if(HAVE_LIBPERFSTAT)
  list(APPEND LIBUV_LIBRARIES perfstat)
endif()

check_library_exists(rt clock_gettime "time.h" HAVE_LIBRT)
if(HAVE_LIBRT)
  list(APPEND LIBUV_LIBRARIES rt)
endif()

check_library_exists(sendfile sendfile "" HAVE_LIBSENDFILE)
if(HAVE_LIBSENDFILE)
  list(APPEND LIBUV_LIBRARIES sendfile)
endif()

if(WIN32)
  # check_library_exists() does not work for Win32 API calls in X86 due to name
  # mangling calling conventions
  list(APPEND LIBUV_LIBRARIES iphlpapi)
  list(APPEND LIBUV_LIBRARIES psapi)
  list(APPEND LIBUV_LIBRARIES userenv)
  list(APPEND LIBUV_LIBRARIES ws2_32)
endif()

if(LIBUV_INCLUDE_DIR AND LIBUV_LIBRARIES)
  set(LIBUV_FOUND TRUE)
else()
  set(LIBUV_FOUND FALSE)
endif()

mark_as_advanced(LIBUV_INCLUDE_DIR LIBUV_LIBRARY)
