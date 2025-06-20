// PakV5-Extract.cpp : PakV5 解包程序
#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <windows.h>
#include <psapi.h>
#include <sstream>  // ✅ 用于 std::stringstream
#include <set>  // ✅ 用于 std::set<std::string>


class IFile;
typedef class IFile* (G_OpenFile)(const char* a1, unsigned int a2, unsigned int a3);
typedef void  (G_SetFilePath)(const char* a1);
typedef void  (G_KMemoryInitialize)(const char* a1, char* a2);
typedef int (G_InitHttpFile)(const char* a1, int);
typedef class IFile* (G_OpenPakV5File)(const char* a1, int*);
typedef int (G_GetPakV5AllFileList)(char**, int64_t*, int*, char*);
typedef char* __cdecl G_setlocale(int Category, const char* Locale);
class IFile
{
public:
	virtual unsigned long	Read(void* Buffer, unsigned long ReadBytes) = 0;
	virtual unsigned long	Write(const void* Buffer, unsigned long WriteBytes) = 0;
	virtual void* GetBuffer() = 0;
	virtual long			Seek(long Offset, int Origin) = 0;
	virtual long			Tell() = 0;
	virtual unsigned long	Size() = 0;
	virtual int				IsFileInPak() = 0;
	virtual int				IsPackedByFragment() = 0;
	virtual int				GetFragmentCount() = 0;
	virtual unsigned int	GetFragmentSize(int nFragmentIndex) = 0;
	virtual unsigned long	ReadFragment(int nFragmentIndex, void*& pBuffer) = 0;
	virtual void			Close() = 0;
	virtual void			Release() = 0;
	virtual ~IFile() {};
};
void setlocal936() {
	HMODULE hMods[4096];
	DWORD cbNeeded;
	if (EnumProcessModules(GetCurrentProcess(), hMods, sizeof(hMods), &cbNeeded))
	{
		for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
		{
			G_setlocale* __setlocale = (G_setlocale*)GetProcAddress(hMods[i], "setlocale");
			if (__setlocale) {
				__setlocale(0, "chinese_china.936");
			}
		}
	}
}
int main(int argc, char** argv)
{
    std::cout << "KS PakV5 Extractor\n";
    std::cout << "                               by Me at 2024.10.10\n";
    std::cout << "usage: \n";
    std::cout << "    1. put with \"configHttpFile.ini\" \"Engine_Lua5X64.dll\" \"KGPK5_FileSystemX64.dll\" \n";
    std::cout << "    2. just run it to extract all, or provide filelist.txt to limit what to extract.\n";

    HMODULE h = LoadLibraryA("Engine_Lua5X64.dll");
    if (h == NULL) {
        std::cout << "bad Engine_Lua5X64.dll\n";
        return -1;
    }
    HMODULE h5 = LoadLibraryA("KGPK5_FileSystemX64.dll");
    if (h5 == NULL) {
        std::cout << "bad KGPK5_FileSystemX64.dll\n";
        return -1;
    }
    G_OpenFile* g_OpenFile = (G_OpenFile*)GetProcAddress(h, "g_OpenFile");
    G_SetFilePath* g_SetRootPath = (G_SetFilePath*)GetProcAddress(h, "g_SetRootPath");
    G_SetFilePath* g_SetFilePath = (G_SetFilePath*)GetProcAddress(h, "g_SetFilePath");
    G_KMemoryInitialize* g_KMemoryInitialize = (G_KMemoryInitialize*)GetProcAddress(h, "?Initialize@KMemory@@YAHQEBD@Z");
    G_InitHttpFile* g_InitHttpFile = (G_InitHttpFile*)GetProcAddress(h5, "g_InitHttpFile");
    G_OpenPakV5File* g_OpenPakV5File = (G_OpenPakV5File*)GetProcAddress(h5, "g_OpenPakV5File");
    G_GetPakV5AllFileList* g_GetPakV5AllFileList = (G_GetPakV5AllFileList*)GetProcAddress(h5, "g_GetPakV5AllFileList");

    char module[4096];
    GetModuleFileNameA(NULL, module, 4096);
    namespace fs = std::filesystem;
    fs::current_path(fs::path(module).remove_filename().string());
    fs::path extract = fs::path(module).replace_extension("extract");

    setlocal936();
    if (g_OpenFile == NULL || g_SetRootPath == NULL || g_SetFilePath == NULL || g_KMemoryInitialize == NULL) {
        std::cout << "bad Engine_Lua5X64.dll\n";
        return -1;
    }
    if (g_InitHttpFile == NULL || g_GetPakV5AllFileList == NULL || g_OpenPakV5File == NULL) {
        std::cout << "bad KGPK5_FileSystemX64.dll\n";
        return -1;
    }

    g_KMemoryInitialize(NULL, NULL);
    g_SetRootPath(NULL);
    g_SetFilePath("");
    int online = 1;
    int v5 = g_InitHttpFile("configHttpFile.ini", online);
    if (!v5) {
        std::cout << "bad configHttpFile.ini\n";
        return -1;
    }

    char* buf = NULL;
    int64_t sze = 0;
    int ver = 0;
    int fl = g_GetPakV5AllFileList(&buf, &sze, &ver, NULL);
    if (!fl) {
        std::cout << "bad FileList\n";
        return -1;
    }

    std::set<std::string> whitelist;

    // ✅ 如果 filelist.txt 不存在，则自动保存完整文件列表
    if (!std::filesystem::exists("filelist.txt")) {
        std::ofstream fout("filelist.txt");
        std::stringstream ss_full(buf);
        std::string full_line;
        int total_count = 0;
        while (std::getline(ss_full, full_line)) {
            if (!full_line.empty() && full_line.back() == '\r') {
                full_line.pop_back();
            }
            fout << full_line << "\n";
            ++total_count;
        }
        fout.close();
        std::cout << "[*] Saved full Pak file list to filelist.txt (" << total_count << " files)\n";
    }
    else {
        std::cout << "[*] filelist.txt already exists. Will use it as whitelist.\n";
    }


    // 🔽 加载 filelist.txt 作为白名单（可选）
    std::ifstream filelist("filelist.txt");
    if (filelist.is_open()) {
        std::string line;
        while (std::getline(filelist, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            whitelist.insert(line);
        }
        filelist.close();
        std::cout << "[*] Will extract only files listed in filelist.txt (" << whitelist.size() << " items)\n";
    }

    // 🔽 解包逻辑
    std::stringstream ss(buf);
    std::string line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        // 跳过非白名单文件
        if (!whitelist.empty() && whitelist.count(line) == 0) {
            continue;
        }

        const char* file = line.c_str();
        fs::path output = extract / file;

        if (fs::exists(output)) {
            std::cout << line << ", skip exist\n";
            continue;
        }

        IFile* f = g_OpenFile(file, 0, 0);
        if (f != NULL) {
            unsigned long sz = f->Size();
            char* m = (char*)malloc(sz + 1);
            unsigned long sz2 = f->Read(m, sz);
            f->Release();

            fs::path parent = output.parent_path();
            if (!fs::exists(parent)) {
                fs::create_directories(parent);
            }

            std::ofstream fs2(output, std::ios::binary);
            fs2.write(m, sz2);
            fs2.close();
            free(m);
            std::cout << line << ", size=" << sz << "\n";
        }
        else {
            std::cout << line << ", open failed\n";
        }
    }

    return 0;
}
