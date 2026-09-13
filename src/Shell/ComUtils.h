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

struct GdiObjectDeleter { void operator()(HGDIOBJ h) const { if (h) DeleteObject(h); } };
struct DcReleaser      { void operator()(HDC h)     const { if (h) ReleaseDC(nullptr, h); } };
struct DcDeleter       { void operator()(HDC h)     const { if (h) DeleteDC(h); } };

template <typename F>
class ScopeGuard {
    F func;
public:
    explicit ScopeGuard(F&& f) : func(std::move(f)) {}
    ~ScopeGuard() { func(); }
};
