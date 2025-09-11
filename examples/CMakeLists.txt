cmake_minimum_required(VERSION 3.16)

# On suppose que la cible header-only cppon::cppon est déjà définie au niveau supérieur.
# Si ce n'est pas le cas (utilisation indépendante), décommente le bloc ci-dessous.
#
# if(NOT TARGET cppon::cppon)
#     add_library(cppon INTERFACE)
#     add_library(cppon::cppon ALIAS cppon)
#     target_include_directories(cppon INTERFACE
#         $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include>
#         $<INSTALL_INTERFACE:include>)
#     target_compile_features(cppon INTERFACE cxx_std_17)
# endif()

file(GLOB CPPON_EXAMPLE_SOURCES
     "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

if(NOT CPPON_EXAMPLE_SOURCES)
    message(STATUS "[cppon] No example sources found in examples/")
endif()

foreach(EX_SRC IN LISTS CPPON_EXAMPLE_SOURCES)
    get_filename_component(EX_NAME "${EX_SRC}" NAME_WE)
    add_executable(${EX_NAME} "${EX_SRC}")
    target_link_libraries(${EX_NAME} PRIVATE cppon::cppon)
    target_compile_features(${EX_NAME} PRIVATE cxx_std_17)
    # Optionnel : warnings cohérents
    if (MSVC)
        target_compile_options(${EX_NAME} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${EX_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endforeach()