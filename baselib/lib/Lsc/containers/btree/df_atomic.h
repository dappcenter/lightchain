
#ifndef __DF_DF_ATOMIC_INCLUDE__
#define __DF_DF_ATOMIC_INCLUDE__

// Proclaim we are using SMP
#ifndef CONFIG_SMP
# define CONFIG_SMP
#endif

#include "df_def.h"

#if defined(__i386__) || defined(__x86_64__)

// Atomic operations that C can't guarantee us.  Useful for
// resource counting etc..
// SMP lock prefix
#ifdef CONFIG_SMP
#define LOCK_PREFIX "lock ; "
#else
#define LOCK_PREFIX ""
#endif

//return: the incremented value;
/// 原子地做 8位，16位，32位，64位的++i的操作
/// 该操作虽然参数和返回值都是无符号型整数，但是一样可以
/// 对有符号型整数做操作，只需要做适当的参数转换即可
#ifdef __x86_64__
static __inline__ uint64_t df_atomic_inc(volatile uint64_t * pv) {
    register unsigned long __res;
    __asm__ __volatile__ (
        "movq $1,%0;"
        LOCK_PREFIX "xaddq %0,(%1);"
        "incq %0"
        :"=a" (__res), "=q" (pv): "1" (pv));
    return __res;
}
#endif

static __inline__ uint32_t df_atomic_inc(volatile uint32_t * pv) {
    register unsigned int __res;
    __asm__ __volatile__ (
        "movl $1,%0;"
        LOCK_PREFIX "xaddl %0,(%1);"
        "incl %0"
        :"=a" (__res), "=q" (pv): "1" (pv));
    return __res;
}

static __inline__ uint16_t df_atomic_inc(volatile uint16_t * pv) {
    register unsigned short __res;
    __asm__ __volatile__ (
        "movw $1,%0;"
        LOCK_PREFIX "xaddw %0,(%1);"
        "incw %0"
        :"=a" (__res), "=q" (pv): "1" (pv));
    return __res;

}

static __inline__ uint8_t  df_atomic_inc(volatile uint8_t * pv) {
    register unsigned char __res;
    __asm__ __volatile__ (
        "movb $1,%0;"
        LOCK_PREFIX "xaddb %0,(%1);"
        "incb %0"
        :"=a" (__res), "=q" (pv): "1" (pv));
    return __res;
}

//return: the decremented value;
/// 原子地做 8位，16位，32位，64位的--i的操作
/// 该操作虽然参数和返回值都是无符号型整数，但是一样可以
/// 对有符号型整数做操作，只需要做适当的参数转换即可
#ifdef __x86_64__
static __inline__ uint64_t df_atomic_dec(volatile uint64_t * pv) {
    register unsigned long __res;
    __asm__ __volatile__ (
        "movq $0xffffffffffffffff,%0;"
        LOCK_PREFIX "xaddq %0,(%1);"
        "decq %0"
        : "=a" (__res), "=q" (pv): "1" (pv));
    return __res;
}
#endif
static __inline__ uint32_t df_atomic_dec(volatile uint32_t * pv) {
    register unsigned int __res;
    __asm__ __volatile__ (
        "movl $0xffffffff,%0;"
        LOCK_PREFIX "xaddl %0,(%1);"
        "decl %0"
        : "=a" (__res), "=q" (pv): "1" (pv));
    return __res;

}
static __inline__ uint16_t df_atomic_dec(volatile uint16_t * pv) {
    register unsigned short __res;
    __asm__ __volatile__ (
        "movw $0xffff,%0;"
        LOCK_PREFIX "xaddw %0,(%1);"
        "decw %0"
        : "=a" (__res), "=q" (pv): "1" (pv));
    return __res;
}

static __inline__ uint8_t  df_atomic_dec(volatile uint8_t * pv) {
    register unsigned char __res;
    __asm__ __volatile__ (
        "movb $0xff,%0;"
        LOCK_PREFIX "xaddb %0,(%1);"
        "decb %0"
        : "=a" (__res), "=q" (pv): "1" (pv));
    return __res;
}

//return: the initial value of *pv
/// 原子地做 8位，16位，32位，64位的加法的操作
/// 该操作虽然参数和返回值都是无符号型整数，但是一样可以
/// 对有符号型整数做操作，只需要做适当的参数转换即可
#ifdef __x86_64__
static __inline__ uint64_t df_atomic_add(volatile uint64_t * pv, const uint64_t av) {
    //:"=a" (__res), "=q" (pv): "m"(av), "1" (pv));
    register unsigned long __res;
    __asm__ __volatile__ (
        "movq %2,%0;"
        LOCK_PREFIX "xaddq %0,(%1);"
        :"=a" (__res), "=q" (pv): "mr"(av), "1" (pv));
    return __res;
}
#endif
static __inline__ uint32_t df_atomic_add(volatile uint32_t * pv, const uint32_t av) {
    //:"=a" (__res), "=q" (pv): "m"(av), "1" (pv));
    register unsigned int __res;
    __asm__ __volatile__ (
        "movl %2,%0;"
        LOCK_PREFIX "xaddl %0,(%1);"
        :"=a" (__res), "=q" (pv): "mr"(av), "1" (pv));
    return __res;
}

static __inline__ uint16_t df_atomic_add(volatile uint16_t * pv, const uint16_t av) {
    //:"=a" (__res), "=q" (pv): "m"(av), "1" (pv));
    register unsigned short __res;
    __asm__ __volatile__ (
        "movw %2,%0;"
        LOCK_PREFIX "xaddw %0,(%1);"
        :"=a" (__res), "=q" (pv): "mr"(av), "1" (pv));
    return __res;
}

static __inline__ uint8_t  df_atomic_add(volatile uint8_t * pv, const uint8_t av) {
    //:"=a" (__res), "=q" (pv): "m"(av), "1" (pv));
    register unsigned char __res;
    __asm__ __volatile__ (
        "movb %2,%0;"
        LOCK_PREFIX "xaddb %0,(%1);"
        :"=a" (__res), "=q" (pv): "mr"(av), "1" (pv));
    return __res;
}

//function: set *pv to nv
//return: the initial value of *pv
/// 原子地把nv赋值给pv指向的整数，支持8位，16位，32位，84位操作
#ifdef __x86_64__
static __inline__ uint64_t df_atomic_exchange(volatile uint64_t * pv, const uint64_t nv) {
    register unsigned long __res;
    __asm__ __volatile__ (
        "1:"
        LOCK_PREFIX "cmpxchgq %3,(%1);"                \
        "jne 1b":
        "=a" (__res), "=q" (pv): "1" (pv), "q" (nv), "0" (*pv));
    return __res;
}
#endif
static __inline__ uint32_t df_atomic_exchange(volatile uint32_t * pv, const uint32_t nv) {
    register unsigned int __res;
    __asm__ __volatile__ (
        "1:"
        LOCK_PREFIX "cmpxchgl %3,(%1);"                \
        "jne 1b":
        "=a" (__res), "=q" (pv): "1" (pv), "q" (nv), "0" (*pv));
    return __res;
}

static __inline__ uint16_t df_atomic_exchange(volatile uint16_t * pv, const uint16_t nv) {
    register unsigned short __res;
    __asm__ __volatile__ (
        "1:"
        LOCK_PREFIX "cmpxchgw %3,(%1);"                \
        "jne 1b":
        "=a" (__res), "=q" (pv): "1" (pv), "q" (nv), "0" (*pv));
    return __res;
}

static __inline__ uint8_t  df_atomic_exchange(volatile uint8_t * pv, const uint8_t nv) {
    register unsigned char __res;
    __asm__ __volatile__ (
        "1:"
        LOCK_PREFIX "cmpxchgb %3,(%1);"                \
        "jne 1b":
        "=a" (__res), "=q" (pv): "1" (pv), "q" (nv), "0" (*pv));
    return __res;
}

//function: compare *pv to cv, if equal, set *pv to nv, otherwise do nothing.
//return: the initial value of *pv
/// 比较pv和cv，如果两者相等，则返回pv原有数值并且把nv赋值给pv
/// 否则什么也不作，返回pv原有数值
#ifdef __x86_64__
static __inline__ uint64_t df_atomic_compare_exchange(volatile uint64_t * pv,
        const uint64_t nv, const uint64_t cv) {
    register unsigned long __res;
    __asm__ __volatile__ (
        LOCK_PREFIX "cmpxchgq %3,(%1)"
        : "=a" (__res), "=q" (pv) : "1" (pv), "q" (nv), "0" (cv));
    return __res;
}
#endif
static __inline__ uint32_t df_atomic_compare_exchange(volatile uint32_t * pv,
        const uint32_t nv, const uint32_t cv) {
    register unsigned int __res;
    __asm__ __volatile__ (
        LOCK_PREFIX "cmpxchgl %3,(%1)"
        : "=a" (__res), "=q" (pv) : "1" (pv), "q" (nv), "0" (cv));
    return __res;
}
static __inline__ uint16_t df_atomic_compare_exchange(volatile uint16_t * pv,
        const uint16_t nv, const uint16_t cv) {
    register unsigned short __res;
    __asm__ __volatile__ (
        LOCK_PREFIX "cmpxchgw %3,(%1)"
        : "=a" (__res), "=q" (pv) : "1" (pv), "q" (nv), "0" (cv));
    return __res;
}
static __inline__ uint8_t df_atomic_compare_exchange(volatile uint8_t * pv,
        const uint8_t nv, const uint8_t cv) {
    register unsigned char  __res;
    __asm__ __volatile__ (
        LOCK_PREFIX "cmpxchgb %3,(%1)"
        : "=a" (__res), "=q" (pv) : "1" (pv), "q" (nv), "0" (cv));
    return __res;
}

typedef void * pvoid;

//function: set *pv to nv
//return: the initial value of *pv
/// 把nv原子地赋值给*pv
static __inline__ pvoid df_atomic_exchange_pointer(volatile pvoid * pv, const pvoid nv) {
#ifdef __x86_64__
    return (pvoid) df_atomic_exchange((uint64_t *) pv, (uint64_t) nv);
#else
    return (pvoid) df_atomic_exchange((uint32_t *) pv, (uint32_t) nv);
#endif
}
//function: compare *pv to cv, if equal, set *pv to nv, otherwise do nothing.
//return: the initial value of *pv
/// 比较cv和*pv，如果两者相等则把nv赋值给*pv，并且返回*pv原有数值
/// 否则返回*pv原有数值，不做赋值操作
static __inline__ pvoid df_atomic_compare_exchange_pointer(volatile pvoid * pv,
        const pvoid nv, const pvoid cv) {
#ifdef __x86_64__
    return (pvoid) df_atomic_compare_exchange((uint64_t *) pv, (uint64_t) nv, (uint64_t)cv);
#else
    return (pvoid) df_atomic_compare_exchange((uint32_t *) pv, (uint32_t) nv, (uint32_t)cv);
#endif
}

#undef LOCK_PREFIX

#else // #if defined(__i386__) || defined(__x86_64__)

#error "must compiled on i386 or x86_64"

#endif //

#endif //__DF_DF_ATOMIC_INCLUDE__
