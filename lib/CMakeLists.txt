set(BUILD_SHARED_LIBS OFF)

#--------------------------------
# Asio
# C++ Async Network Library
#--------------------------------

if(NOT TARGET asio)
    add_library(asio INTERFACE)

    target_include_directories(asio INTERFACE asio/asio/include)

    target_compile_definitions(asio INTERFACE ASIO_HAS_STD_SHARED_PTR=1)
endif()
