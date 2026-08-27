#pragma once

#include <unordered_set>
#include "BasicTypes.h"
#include "WinFramework.h"
#include "ShlObj.h"


// clipboardManager.cpp
void QueryClipBoardCutItems(HWND hwnd, std::unordered_set<u64>& clipBoardCutItems);
void PerformClipboardOperation(PCIDLIST_ABSOLUTE parentPidl, std::vector<PCITEMID_CHILD>&childPidls, bool isCut);


