#pragma once

#include "BasicTypes.h"
#include <unordered_map>
#include "Types/global.h"
 
class TypenameStore {
public:
    using TypeIndex = u16;
    static constexpr TypeIndex InvalidIndex = 0xFFFF;

    // Checks if the UTF-8 string already exists. If not, copies it to the string arena.
    TypeIndex GetOrCreateId(const char* utf8Name) {
        if (!utf8Name) return InvalidIndex;

        u64 hash = HashString(utf8Name);

        // Deduplicate using standard string lookup
        auto it = m_stringToId.find(hash);
        if (it != m_stringToId.end()) {

            TypeIndex existingIndex = it->second;
            if (strcmp(GetTypename(existingIndex), utf8Name) == 0) {    // verify that it is it, incase of collisions
                return existingIndex;
            }
            return it->second;
        }

        //  protection against u16 overflow
        if (m_offsets.size() >= 0xFFFE) {
            return InvalidIndex;
        }

        TypeIndex newIndex = (TypeIndex)m_offsets.size();
        u32 currentOffset = (u32)m_arena.size();
        
        m_offsets.push_back(currentOffset);
        
        // Pack string bytes along with its null-terminator into our arena
        size_t len = strlen(utf8Name) + 1; 

        m_arena.insert(m_arena.end(), utf8Name, utf8Name + len);

        m_stringToId[hash] = newIndex;
        return newIndex;
    }

    const char* GetTypename(TypeIndex index) const {
        if (index < m_offsets.size()) {
            return &m_arena[m_offsets[index]];
        }
        return "";
    }

private:
    std::vector<char> m_arena;               // Flat memory block for sequential strings
    std::vector<uint32_t> m_offsets;         // Offset into the arena for each unique index
    std::unordered_map<u64, TypeIndex> m_stringToId; // Deduplication map
};
