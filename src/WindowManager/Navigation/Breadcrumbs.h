#pragma once

#include <string>
#include "Pidl.h"
#include "BasicTypes.h"
#include <vector>

struct Breadcrumb{
    std::string displayName;
    WShell::Pidl pidl;
    u64 hash;
    
};

struct Breadcrumbs{
    std::string fullPath;
    bool hasSubFolders = false;
    std::vector<Breadcrumb> crumbs; //  list of active crumbs
        
};

Breadcrumbs GenerateBreadcrumbs(PCIDLIST_ABSOLUTE folder);