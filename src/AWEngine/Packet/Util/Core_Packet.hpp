#pragma once

#include <memory>
#include <utility>
#include <functional>

#ifndef AWE_CLASS
#   define AWE_CLASS(awe_name) class awe_name
#endif

#ifndef AWE_STRUCT
#   define AWE_STRUCT(awe_name) struct awe_name
#endif

#ifndef AWE_CLASS_PTR
#   define AWE_CLASS_PTR(awe_name) class awe_name;\
    typedef std::shared_ptr<awe_name> awe_name##_ptr;\
    typedef std::shared_ptr<const awe_name> awe_name##_conptr;\
    typedef std::weak_ptr<awe_name> awe_name##_wptr;\
    typedef std::weak_ptr<const awe_name> awe_name##_conwptr;\
    AWE_CLASS(awe_name)
#endif

#ifndef AWE_STRUCT_PTR
#   define AWE_STRUCT_PTR(awe_name) struct awe_name;\
    typedef std::shared_ptr<awe_name> awe_name##_ptr;\
    typedef std::shared_ptr<const awe_name> awe_name##_conptr;\
    typedef std::weak_ptr<awe_name> awe_name##_wptr;\
    typedef std::weak_ptr<const awe_name> awe_name##_conwptr;\
    AWE_STRUCT(awe_name)
#endif

#ifndef AWE_CLASS_UPTR
#   define AWE_CLASS_UPTR(awe_name) class awe_name;\
    typedef std::unique_ptr<awe_name> awe_name##_uptr;\
    typedef std::unique_ptr<const awe_name> awe_name##_conuptr;\
    AWE_CLASS(awe_name)
#endif

#ifndef AWE_STRUCT_UPTR
#   define AWE_STRUCT_UPTR(awe_name) struct awe_name;\
    typedef std::unique_ptr<awe_name> awe_name##_uptr;\
    typedef std::unique_ptr<const awe_name> awe_name##_conuptr;\
    AWE_STRUCT(awe_name)
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
