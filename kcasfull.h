#pragma once

#include <cassert>
#include <stdint.h>
#include <sstream>
#include <cstring>
#include <limits.h>
using namespace std;

#define CASWORD_BITS_TYPE casword_t
#define casword_t uintptr_t
#define SHIFT_BITS 2
#define CASWORD_CAST(x) ((CASWORD_BITS_TYPE) (x))

// kcas.h

template <typename T>
struct casword {
private:
    CASWORD_BITS_TYPE volatile bits;
public:
    casword();

    T setInitVal(T other);

    operator T();

    T operator->();

    T getValue();

    void addToDescriptor(T oldVal, T newVal);
};

#include "kcas_reuse_htm_impl.h"

namespace kcas {
    KCASHTM<100000> instance;

    void writeInitPtr(casword_t volatile * addr, casword_t const newval) {
        return instance.writeInitPtr(addr, newval);
    }

    void writeInitVal(casword_t volatile * addr, casword_t const newval) {
        return instance.writeInitVal(addr, newval);
    }

    casword_t readPtr(casword_t volatile * addr) {
        return instance.readPtr(addr);
    }

    casword_t readVal(casword_t volatile * addr) {
        return instance.readVal(addr);
    }

    bool execute() {
        return instance.execute();
    }

    kcasptr_t getDescriptor() {
        return instance.getDescriptor();
    }

    void start() {
        return instance.start();
    }

    template<typename T>
    void add(casword<T> * caswordptr, T oldVal, T newVal) {
        return instance.add(caswordptr, oldVal, newVal);
    }

    template<typename T, typename... Args>
    void add(casword<T> * caswordptr, T oldVal, T newVal, Args... args) {
        instance.add(caswordptr, oldVal, newVal, args...);
    }

};


// casword.h
template <typename T>
casword<T>::casword(){
    T a;
    bits =  bits = CASWORD_CAST(a);
}

template <typename T>
T casword<T>::setInitVal(T other) {
    if(is_pointer<T>::value){
	bits = CASWORD_CAST(other);
    }
    else {
	bits = CASWORD_CAST(other);
	assert((bits & 0xE000000000000000) == 0);
	bits = bits << SHIFT_BITS;
    }

    return other;
}

template <typename T>
casword<T>::operator T() {
    if(is_pointer<T>::value){
	return (T)kcas::instance.readPtr(&bits);
    }
    else {
	return (T)kcas::instance.readVal(&bits);
    }
}

template <typename T>
T casword<T>::operator->() {
    assert(is_pointer<T>::value);
    return *this;
}

template <typename T>
T casword<T>::getValue(){
    if(is_pointer<T>::value){
	return (T)kcas::instance.readPtr(&bits);
    }
    else {
	return (T)kcas::instance.readVal(&bits);
    }
}

template <typename T>
void casword<T>::addToDescriptor(T oldVal, T newVal){
    auto descriptor = kcas::instance.getDescriptor();
    auto c_oldVal = (casword_t)oldVal;
    auto c_newVal = (casword_t)newVal;
    assert(((c_oldVal & 0xE000000000000000) == 0) && ((c_newVal & 0xE000000000000000) == 0));

    if(is_pointer<T>::value){
	descriptor->addPtrAddr(&bits, c_oldVal, c_newVal);
    }
    else {
	descriptor->addValAddr(&bits, c_oldVal, c_newVal);
    }
}