# AWEngine - Packet

Library which creates support for sending packets over network.

Developer as part of AWEngine project but can be used independently.

## Libraries

| Name                                                         | License                                          | Version                                                               |
|--------------------------------------------------------------|--------------------------------------------------|-----------------------------------------------------------------------|
| [Asio C++](http://think-async.com/Asio/)                     | [`Boost`](https://www.boost.org/LICENSE_1_0.txt) | branch: [`master`](https://github.com/chriskohlhoff/asio/tree/master) |
| [`portable_endian.h`](https://gist.github.com/panzi/6856583) | `Public Domain`                                  |                                                                       |

All libraries are used as `static library` to maximize optimization and limit problems with deployment and versions.

## Testing

Run `ctest` in build directory.

Executable of tests have prefix `t_` (test `abc` would be executable `t_abc`).
After creating new test, add it to [`tests/CMakeLists.txt`](tests/CMakeLists.txt) as `add_subdirectory(abc)` (where `abc` is name of your test).

To setup tests in CLion IDE, create new Run Configuration with Name=`CTest`, Working Directory=`$CMakeCurrentBuildDir$` and Executable pointing to your `ctest` executable.
On Linux it will be `/bin/ctest`, on Windows probably `C:\CMake\bin\ctest.exe` 

## Platforms

Supported platforms are limited by ASIO C++ and [portable_endian.h](src/portable_endian.h) implementation.

| Platform | Asio C++ | Endian |
|----------|:--------:|:------:|
| Android  | OK       | ✓      |
| FreeBSD  | ✓        | ✓      |
| iOS      | OK       | ✓      |
| Linux    | ✓        | ✓      |
| macOS    | ✓        | ✓      |
| OpenBSD  | OK       | ✓      |
| Windows  | ✓        | ✓      |

`OK` for Asio means that it should work.
For more info, see [Asio's Supported Platforms](https://www.boost.org/doc/libs/develop/doc/html/boost_asio/using.html).

# Limitations

- Maximum 256 packet types in each direction
- Some IDs for packets are reserved
  - Some packets must share same IDs 
    - Server's `Ping`       and client's `Pong`
    - Server's `ServerInfo` and client's `Init`
    - Server's `Kick`       and client's `Disconnect`
  - Universal ServerInfo (game communication initialization)
  - Last 16 IDs (`0xF.`) are reserved for general protocol and future expansion
