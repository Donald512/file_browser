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
    if (!folder){
        return false;
    }

    const i64 eraseFrom = currentIndex + 1;
    if (eraseFrom < 0 || eraseFrom > (i64)visited.size()){
        return false;
    }

    visited.erase(visited.begin() + eraseFrom, visited.end());

    PIDLIST_ABSOLUTE cloned = ILClone(folder);

    if (!cloned){
        return false;
    }

    visited.push_back(WShell::Pidl(cloned));
    currentIndex = (i64)(visited.size() - 1);

    return true;
}   

