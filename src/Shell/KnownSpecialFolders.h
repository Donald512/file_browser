#pragma once
#include "Pidl.h"

// ! Note COM has to be initialized first, so this has to be done in main



// {f874310e-b6b7-47dc-bc84-b9e6b38f5903}
constexpr CLSID CLSID_HOME = 
    { 0xf874310e, 0xb6b7, 0x47dc, { 0xbc, 0x84, 0xb9, 0xe6, 0xb3, 0x8f, 0x59, 0x03 } };

namespace SpecialFolders{
    inline WShell::Pidl pidlThisPC;
    inline WShell::Pidl pidlHome;
    inline WShell::Pidl pidlDesktop;
    inline WShell::Pidl pidlQuickAccess;
    inline WShell::Pidl pidlNetwork;
    inline WShell::Pidl pidlRecycleBin;
    inline WShell::Pidl pidlDocuments;
    inline WShell::Pidl pidlDownloads;
    inline WShell::Pidl defaultStartupFolder;
}


inline void GetSpecialFolders(){
    SHGetKnownFolderIDList(FOLDERID_ComputerFolder, 0, NULL, SpecialFolders::pidlThisPC.GetAddressOf());
    SHGetKnownFolderIDList(FOLDERID_Desktop, 0, NULL, SpecialFolders::pidlDesktop.GetAddressOf()); 
    SHParseDisplayName(L"shell:::{f874310e-b6b7-47dc-bc84-b9e6b38f5903}", NULL, SpecialFolders::pidlHome.GetAddressOf(), 0, NULL);
    SHGetKnownFolderIDList(FOLDERID_NetworkFolder, 0, NULL, SpecialFolders::pidlNetwork.GetAddressOf()); 
    SHGetKnownFolderIDList(FOLDERID_RecycleBinFolder, 0, NULL, SpecialFolders::pidlRecycleBin.GetAddressOf()); 
    SHGetKnownFolderIDList(FOLDERID_Documents, 0, NULL, SpecialFolders::pidlDocuments.GetAddressOf()); 
    SHGetKnownFolderIDList(FOLDERID_Downloads, 0, NULL, SpecialFolders::pidlDownloads.GetAddressOf()); 
    SHParseDisplayName(L"shell:::{679F85CB-0220-4080-B29B-5540CC05AAB6}", NULL, SpecialFolders::pidlQuickAccess.GetAddressOf(), 0, NULL);
    SpecialFolders::defaultStartupFolder = SpecialFolders::pidlThisPC.Clone();
}
