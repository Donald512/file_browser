#include "ClipboardManager.h"

#include <Shlwapi.h>
#include "Pidl.h"
#include "Types/global.h"

// #include <ShlObj.h>
// #include <unordered_set>
// #include "Shell.h"
// #include <iostream>


// inline std::unordered_set<u64> clipBoardCutItems{};

static UINT GetCfDropEffect() {
    static const UINT cf = RegisterClipboardFormat(CFSTR_PREFERREDDROPEFFECT);
    return cf;
}

static UINT GetCfShellIDList() {
    static const UINT cf = RegisterClipboardFormat(CFSTR_SHELLIDLIST);
    return cf;
}

void QueryClipBoardCutItems(HWND hwnd, std::unordered_set<u64>& clipBoardCutItems){

    // ALWAYS clear first. If I copy after cutting, the cut items must be removed.
    clipBoardCutItems.clear();

    if (!OpenClipboard(hwnd)) return;

    HANDLE hDropEffect = GetClipboardData(GetCfDropEffect());
    if (!hDropEffect) {
        CloseClipboard();
        return;
    }

    DWORD* pEffect = static_cast<DWORD*>(GlobalLock(hDropEffect));
    if (!pEffect) {
        CloseClipboard();
        return;
    }

    DWORD effect = *pEffect;
    GlobalUnlock(hDropEffect);


    // If it's not a MOVE (Cut), it's a COPY. We only care about Cut.
    if ((effect & DROPEFFECT_MOVE) == 0) {
        CloseClipboard();
        return;
    }

    HANDLE hShellIDList = GetClipboardData(GetCfShellIDList());
    if (!hShellIDList) {
        CloseClipboard();
        return;
    }

    CIDA* pIDA = static_cast<CIDA*>(GlobalLock(hShellIDList));
    if (pIDA) {
        LPITEMIDLIST parentPidl = reinterpret_cast<LPITEMIDLIST>(reinterpret_cast<BYTE*>(pIDA) + pIDA->aoffset[0]);

        IShellFolder* parentFolder = nullptr;
        if (FAILED(SHBindToObject(nullptr, parentPidl, nullptr, IID_PPV_ARGS(&parentFolder)))){
            GlobalUnlock(hShellIDList);
            CloseClipboard();
            return;
        }

        for (UINT i = 0; i < pIDA->cidl; ++i) {
            LPCITEMIDLIST childPidl = reinterpret_cast<LPCITEMIDLIST>(
                reinterpret_cast<BYTE*>(pIDA) + pIDA->aoffset[i + 1]
            );
            clipBoardCutItems.insert(HashItemIdentity(parentPidl, childPidl));
        }
        parentFolder->Release();
        GlobalUnlock(hShellIDList);
    }

    CloseClipboard();
}

void PerformClipboardOperation(PCIDLIST_ABSOLUTE parentPidl, std::vector<PCITEMID_CHILD>& childPidls, bool isCut){
    IDataObject* pDataObj = nullptr;
    HRESULT hr = SHCreateDataObject(parentPidl, (UINT) childPidls.size(), childPidls.data(), nullptr, IID_PPV_ARGS(&pDataObj));

    if (FAILED(hr) || !pDataObj){
        PRINTERR;
        return;
    }

    FORMATETC fmt = { (CLIPFORMAT)GetCfDropEffect(), NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM med = { TYMED_HGLOBAL, NULL, NULL };
    med.hGlobal = GlobalAlloc(GHND, sizeof(DWORD));

    if (med.hGlobal){
        DWORD* pEffect = (DWORD*)GlobalLock(med.hGlobal);
        *pEffect = isCut ? DROPEFFECT_MOVE : DROPEFFECT_COPY;
        GlobalUnlock(med.hGlobal);

        // TRUE means the IDataObject takes ownership and will free the HGLOBAL
        HRESULT res = pDataObj->SetData(&fmt, &med, TRUE); 
        if (FAILED(res)) {
            std::cout << "SetData failed: 0x" << std::hex << res << std::endl;
        }
        
    }

    HRESULT hrClip = OleSetClipboard(pDataObj);
    if (FAILED(hrClip)) {
        std::cout << "OleSetClipboard failed: 0x" << std::hex << hrClip << std::endl;
    }
    pDataObj->Release();

    OleFlushClipboard();
}