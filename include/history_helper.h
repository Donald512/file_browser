// history_helper.h

#pragma once

#include "file_browser.h"

PathHistory InitHistory();
bool AppendPathToHistory(const String subDir);
bool PopPathHistoryFromIndex(u64 cursorIndex);
bool NavigateForward();
bool NavigateBackward();
bool NewBranch(const String subDir);
