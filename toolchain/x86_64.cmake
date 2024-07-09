set(CMAKE_SYSTEM_NAME Generic)

if(NOT DEFINED ENV{XGCC_BIN})
    message(FATAL_ERROR "No cross toolchain set in \${XGCC_BIN}?")
endif()

if(NOT DEFINED ENV{HOST_GCC_BIN})
    message(FATAL_ERROR "No host toolchain set in \${HOST_GCC_BIN}?")
endif()

set(HOST_C_COMPILER "$ENV{HOST_GCC_BIN}/gcc")
set(HOST_CXX_COMPILER "$ENV{HOST_GCC_BIN}/g++")
set(HOST_LINKER "$ENV{HOST_GCC_BIN}/ld")

set(CMAKE_C_COMPILER "$ENV{XGCC_BIN}/x86_64-elf-gcc")
set(CMAKE_CXX_COMPILER "$ENV{XGCC_BIN}/x86_64-elf-g++")
set(CMAKE_LINKER "$ENV{XGCC_BIN}/x86_64-elf-ld")
set(CMAKE_AR "$ENV{XGCC_BIN}/x86_64-elf-ar")
set(CMAKE_RANLIB "$ENV{XGCC_BIN}/x86_64-elf-ranlib")
set(CMAKE_OBJCOPY "$ENV{XGCC_BIN}/x86_64-elf-objcopy")
set(CMAKE_STRIP "$ENV{XGCC_BIN}/x86_64-elf-strip")
set(CMAKE_SYSROOT "${CMAKE_SOURCE_DIR}/sysroot")
