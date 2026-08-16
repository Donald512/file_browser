#pragma once
#include <memory>
#include <objbase.h>
#include <shtypes.h>

struct CoTaskMemDeleter {
    void operator()(void* p) const {
        CoTaskMemFree(p);
    }
};

// Convenient type aliases:
using UniqueCoTaskStr  = std::unique_ptr<wchar_t, CoTaskMemDeleter>;
using UniquePidl       = std::unique_ptr<ITEMIDLIST, CoTaskMemDeleter>;