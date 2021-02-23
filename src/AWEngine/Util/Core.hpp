#pragma once

#include <memory>

#ifndef AWE_CLASS
    #define AWE_CLASS(awe_name) class awe_name
#endif

#ifndef AWE_STRUCT
    #define AWE_STRUCT(awe_name) struct awe_name
#endif

#ifndef AWE_CLASS_PTR
    #define AWE_CLASS_PTR(awe_name) class awe_name;\
    std::shared_ptr<awe_name> awe_name##_ptr;\
    std::shared_ptr<const awe_name> awe_name##_conptr;\
    std::weak_ptr<awe_name> awe_name##_wptr;\
    std::weak_ptr<const awe_name> awe_name##_conwptr;\
    AWE_CLASS(awe_name)
#endif

#ifdef AWE_STRUCT_PTR
    #define AWE_STRUCT_PTR(awe_name) struct awe_name;\
    std::shared_ptr<awe_name> awe_name##_ptr;\
    std::shared_ptr<const awe_name> awe_name##_conptr;\
    std::weak_ptr<awe_name> awe_name##_wptr;\
    std::weak_ptr<const awe_name> awe_name##_conwptr;\
    AWE_STRUCT(awe_name)
#endif
