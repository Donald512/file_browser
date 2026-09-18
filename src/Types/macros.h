#pragma once


#define PRINTERR \
    do { \
        std::cout << "File: " << __FILE__ << "\n"; \
        std::cout << "Line: " << __LINE__ << "\n"; \
    } while(0)


#define Str8Lit(s) (str8){ (u8 *)(s), sizeof(s) - 1 }

#define CHECK_RET(expr, rv) do { assert(expr); if (!(expr)) return (rv); } while(0)
#define CHECK_VOID(expr)    do { assert(expr); if (!(expr)) return;     } while(0)

#define ExploraMax(a, b) ((a) > (b) ? (a) : (b))
#define ExploraMin(a, b) ((a) < (b) ? (a) : (b))
#define ExploraCeil(numerator, denominator) (((numerator) + (denominator) - 1) /  (denominator))

#define internal static

#define KB(x) ((x) * 1024ULL)
#define MB(x) ((x) * 1024ULL * 1024ULL)
#define GB(x) ((x) * 1024ULL * 1024ULL * 1024ULL)


#define PushStruct(arena, type)             (type*)ArenaPush((arena), 1, sizeof(type), alignof(type))
#define PushArray(arena, type, count)       (type*)ArenaPush((arena), (count), sizeof(type), alignof(type))
#define PushSize(arena, size)               ArenaPush((arena), 1, (size), 8)
#define PushSizeAlign(arena, size, align)   ArenaPush((arena), 1, (size), (align))