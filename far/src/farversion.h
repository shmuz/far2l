#pragma once

extern const uint32_t FAR_VERSION;
extern const char *FAR_BUILD;

#if defined(__x86_64__)
# define FAR_PLATFORM "x64"
#elif defined(__ppc64__)
# define FAR_PLATFORM "ppc64"
#elif defined(__arm64__) || defined(__aarch64__)
# define FAR_PLATFORM "arm64"
#elif defined(__arm__)
# define FAR_PLATFORM "arm"
#elif defined(__e2k__)
# define FAR_PLATFORM "e2k"
#elif defined(__riscv)
# if __riscv_xlen == 64
# define FAR_PLATFORM "rv64"
# else
# define FAR_PLATFORM "rv32"
# endif
#else
# define FAR_PLATFORM "x86"
#endif

