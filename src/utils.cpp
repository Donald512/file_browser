// utils.cpp

#include "core.h"

namespace Utils {
    void InitCOM() {
        // COINIT_APARTMENTTHREADED: Tells Windows this thread will use 
        // the single-threaded apartment model, which is the standard 
        // requirement for UI-heavy applications.
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        
        if (FAILED(hr)) {
            // If it returns RPC_E_CHANGED_MODE, it's already initialized,
            // which is fine. Otherwise, something went wrong.
            if (hr != RPC_E_CHANGED_MODE) {
                PrntErr("Failed to initialize COM");
            }
        }
    }

    void FreePidl(PIDLIST_ABSOLUTE& pidl){
        if (!pidl) return;
        
        ILFree(pidl);
        pidl = nullptr;
    }

    
}