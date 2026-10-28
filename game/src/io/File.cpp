#include "File.h"

#include <filesystem>
#include <fstream>

File::File()
{
    m_path = "";
}

File::File(const std::string &path)
{
    m_path = path;
}

const std::string &File::GetPath() const
{
    return m_path;
}

std::string File::GetName() const
{
    return std::filesystem::path(m_path).filename().string();
}

std::string File::GetExtension() const
{
    return std::filesystem::path(m_path).extension().string();
}

File File::GetParent() const
{
    return File(std::filesystem::path(m_path).parent_path().string());
}

File File::GetChild(const std::string &name) const
{
    return File((std::filesystem::path(m_path) / name).string());
}

bool File::Exists() const
{
    return std::filesystem::exists(m_path);
}

bool File::IsDirectory() const
{
    return std::filesystem::is_directory(m_path);
}

bool File::IsRegularFile() const
{
    return std::filesystem::is_regular_file(m_path);
}

int64_t File::GetModifiedTime() const
{
    std::error_code errorCode;
    std::filesystem::file_time_type writeTime = std::filesystem::last_write_time(m_path, errorCode);
    if (errorCode)
    {
        return 0;
    }

    return (int64_t) writeTime.time_since_epoch().count();
}

bool File::CreateDirectories() const
{
    std::error_code errorCode;
    std::filesystem::create_directories(m_path, errorCode);
    return !errorCode;
}

bool File::Remove() const
{
    std::error_code errorCode;
    std::filesystem::remove(m_path, errorCode);
    return !errorCode;
}

bool File::RemoveAll() const
{
    std::error_code errorCode;
    std::filesystem::remove_all(m_path, errorCode);
    return !errorCode;
}

bool File::RenameTo(const File &target) const
{
    std::error_code errorCode;
    std::filesystem::rename(m_path, target.GetPath(), errorCode);
    return !errorCode;
}

bool File::CopyRecursiveTo(const File &target) const
{
    std::error_code errorCode;
    std::filesystem::copy(m_path, target.GetPath(), std::filesystem::copy_options::recursive,
                          errorCode);
    return !errorCode;
}

bool File::SetOwnerOnlyPermissions() const
{
    std::error_code errorCode;
    std::filesystem::permissions(
            m_path, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
            std::filesystem::perm_options::replace, errorCode);
    return !errorCode;
}

std::vector<File> File::ListFiles() const
{
    std::vector<File> files;
    if (!IsDirectory())
    {
        return files;
    }

    for (const std::filesystem::directory_entry &entry :
         std::filesystem::directory_iterator(m_path))
    {
        files.push_back(File(entry.path().string()));
    }

    return files;
}

std::vector<uint8_t> File::ReadAllBytes() const
{
    std::ifstream stream(m_path, std::ios::binary);
    if (!stream)
    {
        return {};
    }

    stream.seekg(0, std::ios::end);
    std::streamoff size = stream.tellg();
    stream.seekg(0, std::ios::beg);
    if (size <= 0)
    {
        return {};
    }

    std::vector<uint8_t> bytes((size_t) size);
    stream.read((char *) bytes.data(), size);
    return bytes;
}

bool File::WriteAllBytes(const std::vector<uint8_t> &bytes) const
{
    GetParent().CreateDirectories();

    std::ofstream stream(m_path, std::ios::binary | std::ios::trunc);
    if (!stream)
    {
        return false;
    }

    stream.write((const char *) bytes.data(), (std::streamsize) bytes.size());
    return true;
}

std::string File::ReadAllText() const
{
    std::vector<uint8_t> bytes = ReadAllBytes();
    return std::string(bytes.begin(), bytes.end());
}

bool File::AppendText(const std::string &text) const
{
    GetParent().CreateDirectories();

    std::ofstream stream(m_path, std::ios::out | std::ios::app);
    if (!stream)
    {
        return false;
    }

    stream << text;
    return true;
}

bool File::IsEmpty() const
{
    return m_path.empty();
}
