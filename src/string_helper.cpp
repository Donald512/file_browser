// string_helper.cpp


#include "string_helper.h"

wchar_t* Utf8ToWide(const char* utf8, u64 extraCharCount, u64* outNumWideChars){
    // outNumWideChars != numrBytes + extraCharCount
    // an emoji could have len = 2, and 4 bytes
    // so outNumWideChars = 2 + extraCharCount, not 4 plus it, which is why the len has to be calculated in this function, not using string.length
    // if utf8 is a mixture of normal characters and emojis, dividing the length / 2 wont be accurate 
    u64 len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);

    if (!len){
        printf("Error in MultiByteToWideChar. Error: %lu. String: %s\n", GetLastError(), utf8);
        return nullptr;
    }

    u64 totalLen = len + extraCharCount; 
    if (outNumWideChars != nullptr){
        *outNumWideChars = totalLen;
    }

    wchar_t* new_string = (wchar_t*) malloc(sizeof(wchar_t) * totalLen);

    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, new_string, (i32) len); //! must be len, not totalLen

    return new_string;
}

char* WideToUtf8(const wchar_t* wide){
    u64 len = WideCharToMultiByte(CP_UTF8, 0, wide, -1, NULL, 0, NULL, NULL);

    if (!len){
        printf("Error in WideCharToMultiByte. Error: %lu. String: %ls\n", GetLastError(), wide);
        return nullptr;
    }
    // on heap 
    char* new_string = (char*) malloc(sizeof(char) * len);
    if (new_string == nullptr) return nullptr;

    WideCharToMultiByte(CP_UTF8, 0, wide, -1, new_string, (i32) len, 0, 0);

    return new_string;
}

String CreateString(const char* strLiteral){
    String out_str;

    u64 len = strlen(strLiteral);
    out_str.capacity = 64;
    while (len >= out_str.capacity){
        out_str.capacity *= 2;
    }

    // this is going on the heap, not ro. data
    // So it technically is an array, technically..., not a string literal
    out_str.own_str = (char*)malloc(sizeof(char) * out_str.capacity);
    if (out_str.own_str == nullptr){
        printf("malloc failed for %s. capacity: %llu, length: %llu", strLiteral, out_str.capacity, len);
        free(out_str.own_str);
        return String{};
    }
    strcpy_s(out_str.own_str, out_str.capacity, strLiteral);
    out_str.length = len;
    return out_str;
}

void DestroyString(String* target){
    if (target == nullptr){
        PrntErrIsNULL;
        return;
    }
    if( target->own_str == nullptr){
        PrntErrIsNULL;
        return;
    }
    free(target->own_str);
    target->own_str = nullptr;
    target->capacity = 0;
    target->length = 0;
    target = nullptr;
}

String CloneString(const String &source){
    String duplicate;
    duplicate = source;
    // duplicate.own_str points to the same string, it needs its own pointer
    duplicate.own_str = (char*)malloc(sizeof(char) * duplicate.capacity);
    if (duplicate.own_str == nullptr){ 
        PrntErrIsNULL; 
        printf("%s\n",source.own_str); 
        return String{};
    }

    strcpy_s(duplicate.own_str, duplicate.capacity, source.own_str);
    
    return duplicate;
}

void AppendSubDirectory(String* path, const String* subDir){
    u64 requiredLen = path->length + 1 + subDir->length ; // Total length needs to include backslash

    if (requiredLen >= path->capacity){
        while (requiredLen >= path->capacity){
            path->capacity *= 2;
        }

        char* buffer = (char*)realloc(path->own_str, sizeof(char) * path->capacity); 
        if (buffer == nullptr){ PrntErrIsNULL; return;}

        path->own_str = buffer;
    }
    strcat_s(path->own_str, path->capacity, "\\");
    strcat_s(path->own_str, path->capacity, subDir->own_str);

    path->length = requiredLen;

}

void PopPath(String* currentDir){
    u64 length = currentDir->length;

    if (currentDir->own_str[length-1] == ':' && currentDir->length <=3){
        printf("At root, so returning\n");
        return;  // at a root directory
    }

    // i represents position of last backslash
    i64 i;
    for (i = (i64) currentDir->length /*starts at null, hopefully...*/; i >= 0 && currentDir->own_str[i] != '\\'; i--);
    ZeroMemory(currentDir->own_str + i, currentDir->length - i);
    
    currentDir->length = i;
}
