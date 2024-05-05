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

    inline void writeInitPtr(casword_t volatile * addr, casword_t const newval) {
        return instance.writeInitPtr(addr, newval);
    }

    inline void writeInitVal(casword_t volatile * addr, casword_t const newval) {
        return instance.writeInitVal(addr, newval);
    }

    inline casword_t readPtr(casword_t volatile * addr) {
        return instance.readPtr(addr);
    }

    inline casword_t readVal(casword_t volatile * addr) {
        return instance.readVal(addr);
    }

    inline bool execute() {
        return instance.execute();
    }

    inline kcasptr_t getDescriptor() {
        return instance.getDescriptor();
    }

    inline void start() {
        return instance.start();
    }

    template<typename T>
    inline void add(casword<T> * caswordptr, T oldVal, T newVal) {
        return instance.add(caswordptr, oldVal, newVal);
    }

    template<typename T, typename... Args>
    inline void add(casword<T> * caswordptr, T oldVal, T newVal, Args... args) {
        instance.add(caswordptr, oldVal, newVal, args...);
    }

};
