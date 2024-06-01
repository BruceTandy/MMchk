cmake_minimum_required(VERSION 3.16.0)
project(MMchk VERSION 0.1.0 LANGUAGES C)
# set_property(TARGET mytarget PROPERTY C_STANDARD 99)

include(CTest)
enable_testing()
# add_compile_options(-arch x86_64)

add_executable( MMchk MMchk.c
                      MMparams.c
                      MMutility.c
                      MMsortfns.c
              )

target_link_libraries(MMchk m pthread)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)