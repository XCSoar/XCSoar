#include <vector>

_LIBCPP_BEGIN_NAMESPACE_STD
template class __vector_base_common<true>;
_LIBCPP_END_NAMESPACE_STD

#ifdef ANDROID
extern "C" __attribute__((noreturn)) void __cxa_pure_virtual() { abort(); }
#endif
