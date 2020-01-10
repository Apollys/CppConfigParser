#pragma once

#include <ostream>
#include <vector>
#include <string>

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
    os << "[";
    for (size_t i = 0; i < v.size(); ++i) {
        os << v[i];
        if (i + 1 < v.size()) os << ", ";
    }
    os << "]";
    return os;
}

// Specialization for vector of strings
inline std::ostream& operator<<(std::ostream& os, const std::vector<std::string>& v) {
    os << "[";
    for (size_t i = 0; i < v.size(); ++i) {
        os << "\"" << v[i] << "\"";
        if (i + 1 < v.size()) os << ", ";
    }
    os << "]";
    return os;
}
