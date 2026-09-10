#include <thread>
#include "WinFramework.h"
#include <ShlObj.h>
#include <ShlObj_core.h>
#include <wrl/client.h>
#include <vector>
#include <string>

using Microsoft::WRL::ComPtr;

enum class FileOp {Move, Copy, Delete};

struct FileOpParams{
    FileOp type;
    PIDLIST_ABSOLUTE parentFolder = nullptr;    // cloned
    std::vector<PITEMID_CHILD> childItems;      // cloned
    PIDLIST_ABSOLUTE destFolder = nullptr;      // cloned
    DWORD setOpflags = FOF_ALLOWUNDO | FOF_NOCONFIRMMKDIR;
};

void ExecuteAsyncFileOp(HWND hwnd, FileOpParams params ){

    std::thread([hwnd, p = std::move(params)](){
        HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

        if (SUCCEEDED(hr)){
            ComPtr<IFileOperation> pfo;
            if (SUCCEEDED(CoCreateInstance(CLSID_FileOperation, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&pfo)))){
                pfo->SetOperationFlags(p.setOpflags);
                pfo->SetOwnerWindow(hwnd);

                ComPtr<IShellItem> psiDest;
                if (p.destFolder) SHCreateItemFromIDList(p.destFolder, IID_PPV_ARGS(&psiDest));
                
                ComPtr<IShellItem> psiParent;
                if (p.parentFolder) SHCreateItemFromIDList(p.parentFolder, IID_PPV_ARGS(&psiParent));

                if (psiParent){
                    for (PCITEMID_CHILD child : p.childItems){
                        ComPtr<IShellItem> psiChild;
                        if (SUCCEEDED(SHCreateItemWithParent(p.parentFolder, nullptr, child, IID_PPV_ARGS(&psiChild)))){
                            if (p.type == FileOp::Move && psiDest) pfo->MoveItem(psiChild.Get(), psiDest.Get(), nullptr, nullptr);
                            else if (p.type == FileOp::Copy && psiDest) pfo->CopyItem(psiChild.Get(), psiDest.Get(), nullptr, nullptr);
                            else if (p.type == FileOp::Delete)     pfo->DeleteItem(psiChild.Get(), nullptr);
                        }
                        /// do i just use DeleteItems or MoveItems? or CopyItems?
                    }
                    pfo->PerformOperations();
                }
            }
            CoUninitialize();
        }
        if (p.parentFolder) ILFree(p.parentFolder);
        if (p.destFolder) ILFree(p.destFolder);
        for (auto child : p.childItems) {
            if (child) ILFree(child);
        }

    }).detach();
}