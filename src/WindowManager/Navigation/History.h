#pragma once
#include <vector>
#include <ShlObj.h>
#include "BasicTypes.h"
#include "Pidl.h"



class History{
    public:
        bool Push(PCIDLIST_ABSOLUTE folder);
        bool CanGoBack() const { return currentIndex > 0;}
        bool CanGoForward() const { return currentIndex + 1 < (i64) visited.size(); }
        PCIDLIST_ABSOLUTE Current() const { return visited[currentIndex].get(); } 
        bool Back(){
            if (!CanGoBack()) return false;
            currentIndex--;
            return true;
        }
        bool Forward(){
            if (!CanGoForward()) return false;
            currentIndex++;
            return true;
        }
        i64 CurrentIndex() const{
            return currentIndex;
        }
    private:
        std::vector<WShell::Pidl> visited;
        i64 currentIndex = -1;
};


inline bool History::Push(PCIDLIST_ABSOLUTE folder){
    visited.erase(visited.begin() + currentIndex + 1, visited.end());

    visited.push_back(WShell::Pidl(ILClone(folder)));
    currentIndex++;
    return true;
}   

