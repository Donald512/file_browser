#pragma once
#include "Types.h"
#include "Shell.h"


namespace Navigation{
    enum class Actions {
    Normal,     
    Back,   
    Forward,
    Refresh  
    };

    struct Breadcrumb{
        std::string displayName;
       WShell::Pidl pidl;
    };

    class Breadcrumbs{
        public:
            std::string fullPath;
            bool hasSubFolders = false;

            bool Generate(PCIDLIST_ABSOLUTE folder);
            const std::vector<Breadcrumb>& Crumbs() const { return crumbs; }
        private:
            std::vector<Breadcrumb> crumbs; //  list of active crumbs
    };

    class History{
        public:
            bool Push(PCIDLIST_ABSOLUTE folder);
            bool CanGoBack() const { return currentIndex > 0;}
            bool CanGoForward() const { return currentIndex + 1 < (i64) visited.size(); }
            PCIDLIST_ABSOLUTE Current() const { return visited[currentIndex].get();} 
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
        private:
            std::vector<WShell::Pidl> visited;
            i64 currentIndex = -1;
    };

    class NavigationController{
        public:
            bool NavigateTo(PCIDLIST_ABSOLUTE dest, Actions action = Actions::Normal);
            bool CanGoBack() const {return paths.CanGoBack();}
            bool CanGoForward() const {return paths.CanGoForward();}
            bool CanGoParent() const {
                if (!currentFolder || ILIsEmpty(currentFolder.get())) return false;
                return true;
            }
            bool GoBack(){
                if (!paths.Back()) return false;
                return NavigateTo(paths.Current(), Actions::Back);
            }
            bool GoForward(){
                if (!paths.Forward()) return false;
                return NavigateTo(paths.Current(), Actions::Forward);      
            }
            bool GoParent();
            bool Refresh(){
                return NavigateTo(paths.Current(), Actions::Refresh);
            }
    
            PCIDLIST_ABSOLUTE CurrentFolder() const { return currentFolder.get(); }
            WShell::Directory& Contents() { return contents; }
            const WShell::Directory& Contents() const { return contents; }
            
            const Navigation::Breadcrumbs& Breadcrumbs() const { return breadcrumbs; }
    
            
        private:
           WShell::Pidl currentFolder;
           WShell::Directory contents;
            Navigation::Breadcrumbs breadcrumbs;
            Navigation::History paths;
    };

}

