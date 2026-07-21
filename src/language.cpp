#include <core.h>
#include <map>

namespace Lang{

    std::string Translate(AppContext& ctx  , const char* string){
        return translations[ctx.UserLang][string];
    }
}