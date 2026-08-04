#pragma once
#include "Types.h"
#include "Shell.h"
#include <atomic>

// does not need to include AppContext.h because AppContext.h includes it back 
struct TaskSystem;

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
        u64 hash;
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

            // Called once, after AppContext is constructed, before the first NavigateTo. NavigateTo dispactches folder loads to this TaskSystem's thread pool instead of doing them inline
            void BindTaskSystem(TaskSystem& t) { tasks = &t;}

            // True while a navigation's breadcrumbs/contents are being fetched in the background. CurrentFolder()/Breadcrumbs/Contents() still return whatever the PREVIOUS navigation left behind until the new data arrives.
            // td wire into UI (a spinner / dimmed view). but later later
            bool IsLoading() const { return loading;}


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
            TaskSystem* tasks = nullptr;
            std::atomic<bool> loading{ false };
            
            WShell::Pidl currentFolder;
            WShell::Directory contents;
            Navigation::Breadcrumbs breadcrumbs;
            Navigation::History paths;

            // Incremented on every Navigation call. The value at the moment a given navigation was discared, goes with that navigation's background job. when the job finishes and is about to apply its results on the main thread, it compares its captured value against the currentGeneration's value at that time
            std::atomic<u64> currentGeneration{0};

    };

}

