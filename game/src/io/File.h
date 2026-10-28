#pragma once

#include <cstdint>
#include <string>
#include <vector>

class File
{
public:
    File();
    explicit File(const std::string &path);

    const std::string &GetPath() const;
    std::string GetName() const;
    std::string GetExtension() const;
    File GetParent() const;
    File GetChild(const std::string &name) const;

    bool Exists() const;
    bool IsDirectory() const;
    bool IsRegularFile() const;
    int64_t GetModifiedTime() const;
    bool CreateDirectories() const;
    bool Remove() const;
    bool RemoveAll() const;
    bool RenameTo(const File &target) const;
    bool CopyRecursiveTo(const File &target) const;
    bool SetOwnerOnlyPermissions() const;

    std::vector<File> ListFiles() const;
    std::vector<uint8_t> ReadAllBytes() const;
    bool WriteAllBytes(const std::vector<uint8_t> &bytes) const;
    std::string ReadAllText() const;
    bool AppendText(const std::string &text) const;

    bool IsEmpty() const;

private:
    std::string m_path;
};
