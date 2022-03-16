#pragma once

#include <memory>
#include <utility>
#include <functional>
#include <limits>

// Debug breakpoint trigger
#ifdef DEBUG
#   if defined(_MSC_VER)
#       define DEBUG_BREAK __debugbreak();
#   elif defined(__GNUC__)
#       define DEBUG_BREAK __builtin_trap();
#   elif defined(SIGTRAP)
#       define DEBUG_BREAK raise(SIGTRAP);
#   else
#       define DEBUG_BREAK do {} while(0);
#       warning No way to raise debug break.
#   endif
#else
#   define DEBUG_BREAK do {} while(0);
#endif

#ifndef AWE_CLASS_PTR
#   define AWE_CLASS_PTR(awe_name) class awe_name;\
    typedef std::shared_ptr<awe_name> awe_name##_ptr;\
    typedef std::shared_ptr<const awe_name> awe_name##_conptr;\
    typedef std::weak_ptr<awe_name> awe_name##_wptr;\
    typedef std::weak_ptr<const awe_name> awe_name##_conwptr;\
    class awe_name
#endif

#ifndef AWE_STRUCT_PTR
#   define AWE_STRUCT_PTR(awe_name) struct awe_name;\
    typedef std::shared_ptr<awe_name> awe_name##_ptr;\
    typedef std::shared_ptr<const awe_name> awe_name##_conptr;\
    typedef std::weak_ptr<awe_name> awe_name##_wptr;\
    typedef std::weak_ptr<const awe_name> awe_name##_conwptr;\
    struct awe_name
#endif

#ifndef AWE_CLASS_UPTR
#   define AWE_CLASS_UPTR(awe_name) class awe_name;\
    typedef std::unique_ptr<awe_name> awe_name##_uptr;\
    typedef std::unique_ptr<const awe_name> awe_name##_conuptr;\
    class awe_name
#endif

#ifndef AWE_STRUCT_UPTR
#   define AWE_STRUCT_UPTR(awe_name) struct awe_name;\
    typedef std::unique_ptr<awe_name> awe_name##_uptr;\
    typedef std::unique_ptr<const awe_name> awe_name##_conuptr;\
    struct awe_name
#endif

#ifndef AWE_ENUM
#   define AWE_ENUM(enum_name, num_type) \
    enum class enum_name : num_type
#endif

#ifndef AWE_ENUM_FLAGS
#   define AWE_ENUM_FLAGS(enum_name, num_type) \
    enum class enum_name : num_type;\
    inline enum_name operator|(enum_name a, enum_name b)\
    {\
        return static_cast<enum_name>(static_cast<num_type>(a) | static_cast<num_type>(b));\
    }\
    inline bool operator &(enum_name a, enum_name b)\
    {\
        return (static_cast<num_type>(a) & static_cast<num_type>(b)) != 0;\
    }\
    inline enum_name& operator |=(enum_name& a, enum_name b)\
    {\
        return a = a | b;\
    }\
    AWE_ENUM(enum_name, num_type)
#endif



#ifndef AWE_DEBUG_COUT
#   ifdef DEBUG
#       define AWE_DEBUG_COUT(content) std::cout << content << std::endl
#   else
#       define AWE_DEBUG_COUT(content)
#   endif
#endif

#ifndef AWE_DEBUG_CERR
#   ifdef DEBUG
#       define AWE_DEBUG_CERR(content) std::cerr << content << std::endl
#   else
#       define AWE_DEBUG_CERR(content)
#   endif
#endif


namespace AWEngine::Packet
{
    struct NoCopy
    {
        NoCopy() = default;

        NoCopy(const NoCopy&) = delete;
        NoCopy& operator=(const NoCopy&) = delete;
    };

    struct NoMove
    {
        NoMove() = default;

        NoMove(NoMove&&) noexcept = delete;
        NoMove& operator=(NoMove&&) noexcept = delete;
    };

    struct NoCopyOrMove : public NoCopy, public NoMove
    {

    };
}
