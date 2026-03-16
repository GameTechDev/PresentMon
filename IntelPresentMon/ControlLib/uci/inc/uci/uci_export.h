
#ifndef UCI_EXPORT_H
#define UCI_EXPORT_H

#ifdef UCI_STATIC_DEFINE
#  define UCI_EXPORT
#  define UCI_NO_EXPORT
#else
#  ifndef UCI_EXPORT
#    ifdef unified_collector_interface_EXPORTS
        /* We are building this library */
#      define UCI_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define UCI_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef UCI_NO_EXPORT
#    define UCI_NO_EXPORT 
#  endif
#endif

#ifndef UCI_DEPRECATED
#  define UCI_DEPRECATED 
#endif

#ifndef UCI_DEPRECATED_EXPORT
#  define UCI_DEPRECATED_EXPORT UCI_EXPORT UCI_DEPRECATED
#endif

#ifndef UCI_DEPRECATED_NO_EXPORT
#  define UCI_DEPRECATED_NO_EXPORT UCI_NO_EXPORT UCI_DEPRECATED
#endif

/* NOLINTNEXTLINE(readability-avoid-unconditional-preprocessor-if) */
#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef UCI_NO_DEPRECATED
#    define UCI_NO_DEPRECATED
#  endif
#endif

#endif /* UCI_EXPORT_H */
