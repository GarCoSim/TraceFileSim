#ifndef TRACEFILESIM_OPTIONAL_CPP
#define TRACEFILESIM_OPTIONAL_CPP
#include <cstdio>
#include "Optional.hpp"

template <typename T>
Optional<T>::Optional() {
    this->exists = false;
}

template <typename T>
Optional<T>::Optional(T value) {
    this->exists = true;
    this->value = value;
}

template <typename T>
T Optional<T>::getValue() {
    if(exists)
        return value;
    else{
        fprintf(stderr, "Attempt to reify uninitialized value in optional\n");
        throw 19;
    }
}

template <typename T>
bool Optional<T>::isSet() {
    return exists;
}

template <typename T>
void Optional<T>::setValue(T value) {
    this->exists = true;
    this->value = value;
}

template <typename T>
T Optional<T>::getOr(T def) {
    return this->exists ? this->value : def;
}
#endif //TRACEFILESIM_OPTIONAL_CPP