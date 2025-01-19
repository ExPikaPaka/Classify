#pragma once

#include <iostream>
#include <sys/stat.h>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <fileapi.h>
#include <wchar.h>
#undef min
#undef max
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

namespace ent {
    namespace io {

        bool directoryExists(const std::wstring& dirPath) {
            #ifdef _WIN32
            DWORD ftyp = GetFileAttributesW(dirPath.c_str());
            return (ftyp != INVALID_FILE_ATTRIBUTES && (ftyp & FILE_ATTRIBUTE_DIRECTORY));
            #else
            struct stat info;
            return (stat(std::string(dirPath.begin(), dirPath.end()).c_str(), &info) == 0 && (info.st_mode & S_IFDIR));
            #endif
        }

        bool createDirectory(const std::wstring& dirPath) {
            if (!directoryExists(dirPath)) {
                #ifdef _WIN32
                return CreateDirectoryW(dirPath.c_str(), NULL) != 0;
                #else
                return mkdir(std::string(dirPath.begin(), dirPath.end()).c_str(), 0755) == 0; // 0755 is typical permissions
                #endif
            }
            return false; // Directory already exists
        }

        bool clearDirectory(const std::wstring& dirPath, bool debug = false) {
            #ifdef _WIN32
            std::wstring searchPath = dirPath + L"\\*";
            WIN32_FIND_DATAW findData;
            HANDLE hFind = FindFirstFileW(searchPath.c_str(), &findData);

            if (hFind != INVALID_HANDLE_VALUE) {
                do {
                    const std::wstring fileName = findData.cFileName;
                    if (fileName != L"." && fileName != L"..") {
                        std::wstring fullPath = dirPath + L"\\" + fileName;

                        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                            // Recursively delete the contents of the directory
                            clearDirectory(fullPath, debug);
                            RemoveDirectoryW(fullPath.c_str());  // Delete the empty subdirectory
                        } else {
                            DeleteFileW(fullPath.c_str());  // Delete the file
                        }
                    }
                } while (FindNextFileW(hFind, &findData) != 0);
                FindClose(hFind);
                return true; // Success
            } else {
                if (debug) std::wcerr << L"Error accessing directory: " << dirPath << std::endl;
                return false;
            }
            #else // POSIX (Linux/macOS)
            DIR* dir = opendir(std::string(dirPath.begin(), dirPath.end()).c_str());
            if (!dir) {
                if (debug) std::cerr << "Error accessing directory: " << std::string(dirPath.begin(), dirPath.end()) << std::endl;
                return false;
            }

            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                std::string fileName = entry->d_name;
                if (fileName != "." && fileName != "..") {
                    std::string fullPath = std::string(dirPath.begin(), dirPath.end()) + "/" + fileName;

                    struct stat pathStat;
                    stat(fullPath.c_str(), &pathStat);
                    if (S_ISDIR(pathStat.st_mode)) {
                        // Recursively delete the contents of the subdirectory
                        clearDirectory(std::wstring(fullPath.begin(), fullPath.end()), debug);
                        rmdir(fullPath.c_str());  // Delete the empty subdirectory
                    } else {
                        remove(fullPath.c_str());  // Delete the file
                    }
                }
            }
            closedir(dir);
            return true; // Success
            #endif
        }


        bool deleteDirectory(const std::wstring& dirPath) {
            if (directoryExists(dirPath)) {
                #ifdef _WIN32
                return RemoveDirectoryW(dirPath.c_str()) != 0;
                #else
                return rmdir(std::string(dirPath.begin(), dirPath.end()).c_str()) == 0; // rmdir only works if the directory is empty
                #endif
            }
            return false; // Directory does not exist
        }

    }
}
