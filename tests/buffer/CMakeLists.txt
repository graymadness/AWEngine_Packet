add_executable(T_Buffer main.cpp)

target_link_libraries(T_Buffer AWEngine_Packet)

add_test(NAME Buffer COMMAND T_Buffer)
