#pragma once

#if defined(__unix__)
#   include <sys/resource.h>
    /// How many bytes are still available on the stack.
    /// https://man7.org/linux/man-pages/man2/getrlimit.2.html
    [[nodiscard]] inline size_t stackavail() noexcept
    {
        struct rlimit rlim = {};
        if(getrlimit(RLIMIT_STACK, &rlim) != 0)
        {
            std::cerr << "Failed to retrieve stack size. Error: " << errno << std::endl;
            //throw std::runtime_error("Failed to retrieve stack size");
            return 0;
        }
        return rlim.rlim_max - rlim.rlim_cur;
    }
#elif defined(__WIN32)
    /// How many bytes are still available on the stack.
    /// https://stackoverflow.com/a/20930496/10159114
    [[nodiscard]] inline size_t stackavail() noexcept
    {
      // page range
      MEMORY_BASIC_INFORMATION mbi;
      // get range
      VirtualQuery((PVOID)&mbi, &mbi, sizeof(mbi));
      // subtract from top (stack grows downward on win)
      return (UINT_PTR) &mbi-(UINT_PTR)mbi.AllocationBase;
    }
#else

#   warning "Unknown operating system"

#endif
