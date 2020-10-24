set(BUILD_SHARED_LIBS OFF)

#--------------------------------
# Asio
#--------------------------------

add_library(Asio INTERFACE)

target_include_directories(Asio INTERFACE asio/asio/include)
