#pragma once

#include <cassert>
#include <stdint.h>
#include <sstream>
#include <cstring>
#include <limits.h>

#define CASWORD_BITS_TYPE casword_t
#define casword_t uintptr_t
#define SHIFT_BITS 2
#define CASWORD_CAST(x) ((CASWORD_BITS_TYPE) (x))

#include "kcas_reuse_htm_impl.h"

namespace kcas {
    extern KCASHTM<100000> instance;

    void writeInitPtr(uintptr_t volatile * addr, uintptr_t const newval);
    void writeInitVal(uintptr_t volatile * addr, uintptr_t const newval);
    uintptr_t readPtr(uintptr_t volatile * addr);
    uintptr_t readVal(uintptr_t volatile * addr);
    bool execute();
    kcasptr_t getDescriptor();
    void start();

    template<typename T>
    void add(casword<T> * caswordptr, T oldVal, T newVal);

    template<typename T, typename... Args>
    void add(casword<T> * caswordptr, T oldVal, T newVal, Args... args);
};
