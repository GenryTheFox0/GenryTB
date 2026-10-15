// GenryTB_Setup.exe - the one file people download. Plain Win32 (no Qt), the static C runtime: it starts on a clean
// Windows. The program rides inside it (a zip appended by tools/make_release.py; the last 16 bytes: its size, uint64
// LE, + "GENRYTB1").
//   GenryTB_Setup.exe              install (or update) into %LOCALAPPDATA%\Programs\GenryTB, shortcuts on the desktop
//                                  and in Start, an entry in «Apps» (to remove it), then start GenryTB
//   GenryTB_Setup.exe --uninstall  (Windows' «Apps» calls the copy kept in the program's folder) remove it
//   --to "<dir>" another folder; --no-links no shortcuts and no «Apps» entry; --no-run do not start it (checks)
// An update never touches what is the person's: projects\ (their mods) and work\ (settings, caches).
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <objbase.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static HWND g_wnd = nullptr;
static HWND g_text = nullptr;
static const wchar_t* kTitle = L"GenryTB";
static const wchar_t* kUninstallKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\GenryTB";

static void say(const wchar_t* t)
{
    if (g_text) SetWindowTextW(g_text, t);
    MSG m;
    while (PeekMessageW(&m, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }
}

static std::wstring knownFolder(REFKNOWNFOLDERID id)
{
    PWSTR p = nullptr;
    std::wstring out;
    if (SUCCEEDED(SHGetKnownFolderPath(id, 0, nullptr, &p))) out = p;
    CoTaskMemFree(p);
    return out;
}

static DWORD runAndWait(const std::wstring& cmd, const std::wstring& dir, bool hidden)
{
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    if (hidden) {
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow = SW_HIDE;
    }
    PROCESS_INFORMATION pi{};
    std::wstring c = cmd;
    if (!CreateProcessW(nullptr, c.data(), nullptr, nullptr, FALSE, hidden ? CREATE_NO_WINDOW : 0, nullptr, dir.c_str(), &si, &pi))
        return 0xFFFFFFFF;
    while (WaitForSingleObject(pi.hProcess, 50) == WAIT_TIMEOUT) say(nullptr);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return code;
}

static bool shortcut(const std::wstring& lnk, const std::wstring& target, const std::wstring& dir, const std::wstring& desc)
{
    IShellLinkW* sl = nullptr;
    if (FAILED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void**>(&sl)))) return false;
    sl->SetPath(target.c_str());
    sl->SetWorkingDirectory(dir.c_str());
    sl->SetDescription(desc.c_str());
    sl->SetIconLocation(target.c_str(), 0);
    IPersistFile* pf = nullptr;
    bool ok = false;
    if (SUCCEEDED(sl->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&pf)))) {
        ok = SUCCEEDED(pf->Save(lnk.c_str(), TRUE));
        pf->Release();
    }
    sl->Release();
    return ok;
}

// the program's own files over the old ones; what is the person's (projects\, work\) stays
static void copyOver(const fs::path& from, const fs::path& to)
{
    std::error_code ec;
    for (const auto& e : fs::recursive_directory_iterator(from, ec)) {
        const fs::path rel = fs::relative(e.path(), from, ec);
        const fs::path dst = to / rel;
        if (e.is_directory()) {
            fs::create_directories(dst, ec);
        } else {
            fs::create_directories(dst.parent_path(), ec);
            fs::copy_file(e.path(), dst, fs::copy_options::overwrite_existing, ec);
        }
    }
}

static LRESULT CALLBACK wndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    if (m == WM_CTLCOLORSTATIC) {
        HDC dc = reinterpret_cast<HDC>(w);
        SetTextColor(dc, RGB(236, 236, 236));
        SetBkColor(dc, RGB(14, 14, 16));
        static HBRUSH b = CreateSolidBrush(RGB(14, 14, 16));
        return reinterpret_cast<LRESULT>(b);
    }
    if (m == WM_ERASEBKGND) {
        RECT r;
        GetClientRect(h, &r);
        static HBRUSH b = CreateSolidBrush(RGB(14, 14, 16));
        FillRect(reinterpret_cast<HDC>(w), &r, b);
        return 1;
    }
    return DefWindowProcW(h, m, w, l);
}

static void window(HINSTANCE inst)
{
    WNDCLASSW wc{};
    wc.lpfnWndProc = wndProc;
    wc.hInstance = inst;
    wc.lpszClassName = L"GenryTBSetup";
    wc.hIcon = LoadIconW(inst, L"IDI_ICON1");
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&wc);
    const int W = 460, H = 120;
    g_wnd = CreateWindowExW(WS_EX_TOPMOST, wc.lpszClassName, kTitle, WS_POPUP | WS_BORDER, (GetSystemMetrics(SM_CXSCREEN) - W) / 2,
                            (GetSystemMetrics(SM_CYSCREEN) - H) / 2, W, H, nullptr, nullptr, inst, nullptr);
    HFONT big = CreateFontW(30, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    HFONT smallFont = CreateFontW(18, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, CLEARTYPE_QUALITY, 0, L"Segoe UI");
    HWND t1 = CreateWindowW(L"STATIC", L"GenryTB", WS_CHILD | WS_VISIBLE, 24, 18, 400, 36, g_wnd, nullptr, inst, nullptr);
    g_text = CreateWindowW(L"STATIC", L"", WS_CHILD | WS_VISIBLE, 24, 62, 420, 40, g_wnd, nullptr, inst, nullptr);
    SendMessageW(t1, WM_SETFONT, reinterpret_cast<WPARAM>(big), TRUE);
    SendMessageW(g_text, WM_SETFONT, reinterpret_cast<WPARAM>(smallFont), TRUE);
    ShowWindow(g_wnd, SW_SHOW);
    UpdateWindow(g_wnd);
}

static int uninstall(const fs::path& dir)
{
    const int keep = MessageBoxW(nullptr, L"Удалить GenryTB?\n\nТвои моды (папка projects) — тоже удалить?\n\n«Да» — удалить всё, «Нет» — оставить моды, «Отмена» — ничего не трогать.",
                                 kTitle, MB_YESNOCANCEL | MB_ICONQUESTION);
    if (keep == IDCANCEL) return 0;
    std::error_code ec;
    fs::remove(fs::path(knownFolder(FOLDERID_Desktop)) / L"GenryTB.lnk", ec);
    fs::remove(fs::path(knownFolder(FOLDERID_Programs)) / L"GenryTB.lnk", ec);
    RegDeleteTreeW(HKEY_CURRENT_USER, kUninstallKey);
    for (const wchar_t* d : {L"app", L"data", L"work"}) fs::remove_all(dir / d, ec);
    fs::remove(dir / L"ПРОЧТИ.txt", ec);
    if (keep == IDYES) fs::remove_all(dir / L"projects", ec);
    // this very file cannot delete itself while it runs: cmd removes it (and the folder, if empty) a moment later
    const std::wstring self = (dir / L"uninstall.exe").wstring();
    const std::wstring cmd = L"cmd.exe /c ping -n 3 127.0.0.1 >nul & del /f /q \"" + self + L"\" & rd \"" + dir.wstring() + L"\"";
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::wstring c = cmd;
    if (CreateProcessW(nullptr, c.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    MessageBoxW(nullptr, keep == IDYES ? L"GenryTB удалён." : L"GenryTB удалён. Твои моды остались в папке projects.", kTitle, MB_ICONINFORMATION);
    return 0;
}

int WINAPI wWinMain(HINSTANCE inst, HINSTANCE, PWSTR cmdLine, int)
{
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    wchar_t me[MAX_PATH];
    GetModuleFileNameW(nullptr, me, MAX_PATH);
    fs::path target = fs::path(knownFolder(FOLDERID_LocalAppData)) / L"Programs" / L"GenryTB";
    if (cmdLine && wcsstr(cmdLine, L"--uninstall")) return uninstall(fs::path(me).parent_path());
    bool links = true, run = true;
    {
        int argc = 0;
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        for (int i = 1; argv && i < argc; ++i) {
            if (wcscmp(argv[i], L"--to") == 0 && i + 1 < argc) target = argv[++i];
            else if (wcscmp(argv[i], L"--no-links") == 0) links = false;
            else if (wcscmp(argv[i], L"--no-run") == 0) run = false;
        }
        LocalFree(argv);
    }

    // the payload: the zip at the end of this very file
    std::ifstream self(me, std::ios::binary);
    self.seekg(-16, std::ios::end);
    uint64_t size = 0;
    char magic[8] = {};
    self.read(reinterpret_cast<char*>(&size), 8);
    self.read(magic, 8);
    const std::streamoff total = self.tellg();
    if (std::string(magic, 8) != "GENRYTB1" || size == 0 || static_cast<std::streamoff>(size) > total - 16) {
        MessageBoxW(nullptr, L"Установщик повреждён: внутри нет программы. Скачай GenryTB_Setup.exe заново.", kTitle, MB_ICONERROR);
        return 1;
    }
    window(inst);
    say(L"Распаковываю…");
    wchar_t tmpBase[MAX_PATH];
    GetTempPathW(MAX_PATH, tmpBase);
    const fs::path tmp = fs::path(tmpBase) / (L"GenryTB_Setup_" + std::to_wstring(GetTickCount64()));
    std::error_code ec;
    fs::create_directories(tmp, ec);
    const fs::path zip = tmp / L"payload.zip";
    {
        std::ofstream out(zip, std::ios::binary);
        self.seekg(total - 16 - static_cast<std::streamoff>(size));
        std::vector<char> buf(1 << 20);
        uint64_t left = size;
        while (left) {
            const size_t n = static_cast<size_t>(left < buf.size() ? left : buf.size());
            self.read(buf.data(), n);
            out.write(buf.data(), n);
            left -= n;
        }
    }
    wchar_t sys[MAX_PATH];
    GetSystemDirectoryW(sys, MAX_PATH);
    const std::wstring tar = std::wstring(sys) + L"\\tar.exe";
    if (GetFileAttributesW(tar.c_str()) == INVALID_FILE_ATTRIBUTES ||
        runAndWait(L"\"" + tar + L"\" -xf \"" + zip.wstring() + L"\" -C \"" + tmp.wstring() + L"\"", tmp.wstring(), true) != 0) {
        MessageBoxW(g_wnd, L"Не получилось распаковать программу (нужна Windows 10 или новее).", kTitle, MB_ICONERROR);
        fs::remove_all(tmp, ec);
        return 1;
    }
    // GenryTB open from this folder: an update cannot replace its files - ask to close it
    say(L"Устанавливаю…");
    while (true) {
        const fs::path exe = target / L"app" / L"GenryTB.exe";
        HANDLE h = CreateFileW(exe.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
        if (h == INVALID_HANDLE_VALUE && GetLastError() == ERROR_SHARING_VIOLATION) {
            if (MessageBoxW(g_wnd, L"GenryTB сейчас открыт. Закрой его и нажми «Повтор».", kTitle, MB_RETRYCANCEL | MB_ICONWARNING) == IDCANCEL) {
                fs::remove_all(tmp, ec);
                return 1;
            }
            continue;
        }
        if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
        break;
    }
    const fs::path src = tmp / L"GenryTB";
    fs::create_directories(target, ec);
    fs::remove_all(target / L"app", ec);                   // the old runtime goes whole: no stale DLLs left behind
    for (const wchar_t* d : {L"app", L"data"}) copyOver(src / d, target / d);
    fs::copy_file(src / L"ПРОЧТИ.txt", target / L"ПРОЧТИ.txt", fs::copy_options::overwrite_existing, ec);
    // the sample mods: only those the person does not have yet (theirs are never overwritten)
    std::error_code ec2;
    for (const auto& e : fs::directory_iterator(src / L"projects", ec2)) {
        const fs::path dst = target / L"projects" / e.path().filename();
        if (!fs::exists(dst)) copyOver(e.path(), dst);
    }
    // the uninstaller: this setup without its payload
    {
        std::ifstream in(me, std::ios::binary);
        std::ofstream out(target / L"uninstall.exe", std::ios::binary | std::ios::trunc);
        std::vector<char> buf(static_cast<size_t>(total - 16 - static_cast<std::streamoff>(size)));
        in.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        out.write(buf.data(), static_cast<std::streamsize>(buf.size()));
    }
    const std::wstring exe = (target / L"app" / L"GenryTB.exe").wstring();
    const std::wstring appDir = (target / L"app").wstring();
    if (links) {
        shortcut((fs::path(knownFolder(FOLDERID_Desktop)) / L"GenryTB.lnk").wstring(), exe, appDir, L"GenryTB — конструктор модов «Зайчика»");
        shortcut((fs::path(knownFolder(FOLDERID_Programs)) / L"GenryTB.lnk").wstring(), exe, appDir, L"GenryTB — конструктор модов «Зайчика»");
    }
    HKEY k;
    if (links && RegCreateKeyExW(HKEY_CURRENT_USER, kUninstallKey, 0, nullptr, 0, KEY_WRITE, nullptr, &k, nullptr) == ERROR_SUCCESS) {
        auto put = [&](const wchar_t* name, const std::wstring& v) {
            RegSetValueExW(k, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(v.c_str()), static_cast<DWORD>((v.size() + 1) * sizeof(wchar_t)));
        };
        put(L"DisplayName", L"GenryTB — конструктор модов «Зайчика»");
        put(L"DisplayVersion", L"1.0.0");
        put(L"Publisher", L"GenryTheFox");
        put(L"DisplayIcon", exe);
        put(L"InstallLocation", target.wstring());
        put(L"UninstallString", L"\"" + (target / L"uninstall.exe").wstring() + L"\" --uninstall");
        const DWORD one = 1;
        RegSetValueExW(k, L"NoModify", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&one), sizeof(one));
        RegSetValueExW(k, L"NoRepair", 0, REG_DWORD, reinterpret_cast<const BYTE*>(&one), sizeof(one));
        RegCloseKey(k);
    }
    fs::remove_all(tmp, ec);
    say(run ? L"Готово. Запускаю GenryTB…" : L"Готово.");
    if (!run) {
        Sleep(600);
        DestroyWindow(g_wnd);
        CoUninitialize();
        return 0;
    }
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::wstring c = L"\"" + exe + L"\"";
    if (CreateProcessW(nullptr, c.data(), nullptr, nullptr, FALSE, 0, nullptr, appDir.c_str(), &si, &pi)) {
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
    Sleep(900);
    DestroyWindow(g_wnd);
    CoUninitialize();
    return 0;
}
