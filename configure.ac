#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.



AC_PREREQ([2.69])
AC_INIT([tomahawk], [0.6.1], [BUG-REPORT-ADDRESS])
: ${CXXFLAGS=-O3}
AC_CONFIG_MACRO_DIRS([m4])
AC_CONFIG_AUX_DIR([build-aux])


AC_ENABLE_SHARED
AC_DISABLE_STATIC
LT_INIT

AM_INIT_AUTOMAKE([foreign subdir-objects])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([include/config.h])

AC_DEFINE( [TOMAHAWK_VERSION_MAJOR], [0], [Major version])
AC_DEFINE( [TOMAHAWK_VERSION_MINOR], [7], [Minor version])
AC_DEFINE( [TOMAHAWK_VERSION_PATCH], [1], [Patch version])


AC_PROG_CC_C99

AC_LANG([C++])
# Checks for programs.
AC_PROG_CXX



AX_CXX_COMPILE_STDCXX_11

AX_CHECK_X86_FEATURES

if test "$HAVE_SSE4_2_INSTRUCTIONS"; then
   AC_MSG_RESULT(["X86 Features: $X86_FEATURE_CFLAGS"])
else
   AC_MSG_ERROR(["SSE4.2 feature not found! X86_Features: $X86_FEATURE_CFLAGS"])
fi




# FIXME: Replace `main' with a function in `-lhts':
PKG_CHECK_MODULES([ZSTD], libzstd, [
    LIBS="$ZSTD_LIBS $LIBS"
    CXXFLAGS="$ZSTD_CFLAGS $CXXFLAGS"
    AC_DEFINE([WITH_ZSTD], [1], [Use zstd])
  ], [
    AC_SEARCH_LIBS(ZSTD_decompressStream, zstd,

      [AC_MSG_ERROR(libzstd was not found)])
  ])

PKG_CHECK_MODULES([XXHASH], libxxhash, [
    LIBS="XXHASH_LIBS $LIBS"
    CXXFLAGS="$XXHASH_CFLAGS $CXXFLAGS"
  ], [
    AC_SEARCH_LIBS(XXH64_update, xxhash,[

],

      [AC_MSG_ERROR(xxhash was not found)])
  ])


PKG_CHECK_MODULES([CRYPTO], libcrypto, [
    LIBS="$CRYPTO_LIBS $LIBS"
    CXXFLAGS="$CRYPTO_CFLAGS $CXXFLAGS"
    AC_DEFINE([WITH_CRYPTO], [1], [Use crypto])
  ], [
    AC_SEARCH_LIBS(ZSTD_decompressStream, crypto,

      [AC_MSG_ERROR(cryptolib was not found)])
  ])



PKG_CHECK_MODULES([HTS], htslib, [
    LIBS="$HTS_LIBS $LIBS"
    CXXFLAGS="$HTS_CFLAGS $CXXFLAGS"
  ], [
    AC_SEARCH_LIBS(bcf_init1, hts,

      [AC_MSG_ERROR(htslib was not found)])
  ])




CXXFLAGS="-I./src $CXXFLAGS"

# Checks for header files.
AC_CHECK_HEADERS([inttypes.h stdint.h stdlib.h strings.h sys/time.h pthread.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT16_T
AC_TYPE_INT32_T
AC_TYPE_INT64_T
AC_TYPE_INT8_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
AC_CHECK_TYPES([ptrdiff_t])

dnl # Checks for library functions.
AC_CHECK_FUNCS([gettimeofday memset pow strncasecmp])
AC_SUBST(LIBS)
AC_SUBST(CXXFLAGS)
AC_SUBST(X86_FEATURE_CFLAGS)
AC_SUBST(CFLAGS)

AC_DEFINE( [VERSION], [PACKAGE_VERSION], [Patch version])
AC_CONFIG_FILES([Makefile])

# Result of conf to screen
AC_MSG_RESULT([
--------------------------------------------------
Configuration summary for $PACKAGE_NAME ($VERSION)
--------------------------------------------------
CC                  : $CC
CPPFLAGS            : $CPPFLAGS
CXXFLAGS            : $CXXFLAGS
CXX                 : $CXX
---
LDFLAGS             : $LDFLAGS
LIBS                : $LIBS
---
Installation prefix : $prefix
$LIBRARY_RESULT
$SUMMARY_RESULT
--------------------------------------------------
Configuration OK
--------------------------------------------------
])



#AC_CONFIG_FILES([makefile])
AC_OUTPUT
