#pragma once

#include "App.h"
#include "imgui_render_boilerplate.h"

class FileDropTarget : public IDropTarget {
    App& m_app;
    ULONG m_cRef;
    bool m_isValidFormat = false;    // Tracks CF_HDROP validity from DragEnter

public:
    FileDropTarget(App& app) : m_app(app), m_cRef(1) {}

    // IUnknown (Use Interlocked for thread safety)
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (!ppv) return E_POINTER;
        if (riid == IID_IUnknown || riid == IID_IDropTarget) { 
            *ppv = static_cast<IDropTarget*>(this); AddRef(); return S_OK; 
        }
        *ppv = nullptr; return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_cRef); }
    ULONG STDMETHODCALLTYPE Release() override { 
        ULONG r = InterlockedDecrement(&m_cRef); if (r == 0) delete this; return r; 
    }

    HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
        if (!pdwEffect) return E_POINTER;

        // Validate format ONCE here
        FORMATETC fmt = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        m_isValidFormat = (pDataObj && pDataObj->QueryGetData(&fmt) == S_OK);

        return HandleDrag(pDataObj, grfKeyState, pt, pdwEffect);
    }

    HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
        return HandleDrag(nullptr, grfKeyState, pt, pdwEffect);
    }

    HRESULT STDMETHODCALLTYPE DragLeave() override {
        m_app.dragInfo.isExternalDragActive = false;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override {
        (void)pt;
        if (!pdwEffect) return E_POINTER;
        *pdwEffect = DROPEFFECT_NONE;
        m_app.dragInfo.isExternalDragActive = false;

        if (!pDataObj) return S_OK;

        // 1. Get Destination from UI state (updated during the last render)
        PIDLIST_ABSOLUTE destPidl = nullptr;

        switch(m_app.dragInfo.targetView){
            case DragInfo::DropView::FolderItem:{
                DirListing listing = GetVisibleListing(m_app);

                if (m_app.dragInfo.dropTargetHash.has_value()){
                    if (listing.PChildren){
                        size_t visualIndex = GetVisualIndexFromHash(listing, *m_app.dragInfo.dropTargetHash);
                        bool isValid = visualIndex < listing.refs.size();
                        assert(isValid);
                        if (isValid){
                            auto rawIndex = listing.refs[visualIndex];
                            auto child = listing.PChildren->GetItem(rawIndex);
                            destPidl = GetFullPidl(listing.dir.parent.pidl.get(), child.pidl);
                        }
                    }
                }
                break;
            }   
            case DragInfo::DropView::CurrentDir:{
                DirListing listing = GetVisibleListing(m_app);
                destPidl = ILClone(listing.dir.parent.pidl.get());
            }
        }
        m_app.dragInfo.targetView = DragInfo::DropView::None;
        m_app.dragInfo.dropTargetHash = std::nullopt;

        if (!destPidl) return S_OK; // Dropped on invalid area (e.g. over a non-folder file)

        // 2. Extract Source files from CF_HDROP
        FORMATETC fmt = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        STGMEDIUM med;
        if (SUCCEEDED(pDataObj->GetData(&fmt, &med))) {
            HDROP hDrop = static_cast<HDROP>(GlobalLock(med.hGlobal));
            if (hDrop) {
                UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
                if (count > 0) {
                    wchar_t path[MAX_PATH];
                    DragQueryFileW(hDrop, 0, path, MAX_PATH);
                    PIDLIST_ABSOLUTE sourceParentPidl = nullptr;
                    
                    std::vector<PITEMID_CHILD> childPidls;
                    childPidls.reserve(count);

                    for (UINT i = 0; i < count; i++) {
                        DragQueryFileW(hDrop, i, path, MAX_PATH);
                        PIDLIST_ABSOLUTE absPidl = ILCreateFromPathW(path);
                        if (absPidl) {
                            if (!sourceParentPidl){
                                sourceParentPidl = ILClone(absPidl);
                                ILRemoveLastID(sourceParentPidl);
                            }
                            LPCITEMIDLIST child = ILFindLastID(absPidl);
                            childPidls.push_back(ILClone(child));
                            ILFree(absPidl);
                        }
                    }

                    if (sourceParentPidl && !childPidls.empty()) {
                        bool isCopy = (grfKeyState & MK_CONTROL) != 0;
                        char buf[64]; wsprintfA(buf, "Drop: ctrl=%d isCopy=%d\n", !!(grfKeyState & MK_CONTROL), !!(grfKeyState & MK_CONTROL));
                        printf("%s", buf);
                        *pdwEffect = isCopy ? DROPEFFECT_COPY : DROPEFFECT_MOVE;

                        Cmd_CopyOrMoveItems cmd;
                        cmd.parentPidl = sourceParentPidl; 
                        cmd.items = std::move(childPidls);
                        cmd.targetPidl = destPidl;         
                        cmd.isCopy = isCopy;
                        m_app.cmdQueue.QueueCommand(std::move(cmd));
                        
                        GlobalUnlock(med.hGlobal);
                        ReleaseStgMedium(&med);
                        return S_OK; 
                    }
                    // If execution reaches here, queuing failed — free allocated source PIDLs
                    if (sourceParentPidl) ILFree(sourceParentPidl);
                    for (PITEMID_CHILD childPidl : childPidls) {
                        ILFree(childPidl);
                    }
                }
                GlobalUnlock(med.hGlobal);
            }
            ReleaseStgMedium(&med);
        }

        if (destPidl) ILFree(destPidl);
        return S_OK;
    }

private:
    HRESULT HandleDrag(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) {
        if (!pdwEffect) return E_POINTER;

        // Reject immediately if DragEnter determined the format was invalid
        if (!m_isValidFormat) {
            *pdwEffect = DROPEFFECT_NONE;
            return S_OK;
        }

        if (pDataObj) {
            FORMATETC fmt = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
            if (pDataObj->QueryGetData(&fmt) != S_OK) {
                *pdwEffect = DROPEFFECT_NONE;
                return S_OK;
            }
        }

        // Inject mouse position into ImGui so IsItemHovered() works!
        POINT clientPt = { pt.x, pt.y };
        ScreenToClient(m_app.gfx.hwnd, &clientPt);
        ImGui::GetIO().MousePos = ImVec2((f32)clientPt.x, (f32)clientPt.y);

        if (m_app.dragInfo.isInternalDragActive) {
            // INTERNAL DRAG: Main loop is blocked. Force a render to update UI state.
            ImGui::GetIO().MouseDown[ImGuiMouseButton_Left] = false; // Prevent phantom clicks
            RenderFrame(m_app); 
            ImGui::GetIO().MouseDown[ImGuiMouseButton_Left] = true;  
        } 
        else {
            // EXTERNAL DRAG: Main loop is running naturally. Just set the flag.
            m_app.dragInfo.isExternalDragActive = true;
        }

        DWORD allowedSrcEffects = *pdwEffect;
        bool hasTarget = (m_app.dragInfo.dropTargetHash.has_value());
        if (!hasTarget){
            *pdwEffect = DROPEFFECT_NONE;
            return S_OK;
        }

        DWORD desiredEffect = (grfKeyState & MK_CONTROL) ? DROPEFFECT_COPY : DROPEFFECT_MOVE;

        if (allowedSrcEffects & desiredEffect){
            *pdwEffect = desiredEffect;
        } else if (allowedSrcEffects & DROPEFFECT_COPY) {
            *pdwEffect = DROPEFFECT_COPY;
        } else {
            *pdwEffect = DROPEFFECT_NONE;
        }
        return S_OK;
    }
};

