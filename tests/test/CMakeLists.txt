add_executable(T_Test main.cpp)

target_link_libraries(T_Test AWEngine_Packet)

add_test(NAME Test COMMAND T_Test)
