#pragma once

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "ListTag.h"
#include "Tag.h"

class CompoundTag : public Tag
{
public:
    CompoundTag();

    eTAG_TYPE GetType() const override;
    void Read(DataInputStream *inputStream) override;
    void Write(DataOutputStream *outputStream) const override;
    std::unique_ptr<Tag> Clone() const override;

    void Put(const std::string &name, std::unique_ptr<Tag> tag);
    void PutByte(const std::string &name, int8_t value);
    void PutShort(const std::string &name, int16_t value);
    void PutInt(const std::string &name, int value);
    void PutLong(const std::string &name, int64_t value);
    void PutFloat(const std::string &name, float value);
    void PutDouble(const std::string &name, double value);
    void PutString(const std::string &name, const std::string &value);
    void PutByteArray(const std::string &name, const std::vector<uint8_t> &value);
    void PutIntArray(const std::string &name, const std::vector<int> &value);
    void PutLongArray(const std::string &name, const std::vector<int64_t> &value);
    void PutBoolean(const std::string &name, bool value);

    Tag *Get(const std::string &name) const;
    bool Contains(const std::string &name) const;
    bool Contains(const std::string &name, eTAG_TYPE type) const;
    int8_t GetByte(const std::string &name) const;
    int16_t GetShort(const std::string &name) const;
    int GetInt(const std::string &name) const;
    int64_t GetLong(const std::string &name) const;
    float GetFloat(const std::string &name) const;
    double GetDouble(const std::string &name) const;
    double GetNumber(const std::string &name) const;
    std::string GetString(const std::string &name) const;
    std::vector<uint8_t> GetByteArray(const std::string &name) const;
    std::vector<int> GetIntArray(const std::string &name) const;
    std::vector<int64_t> GetLongArray(const std::string &name) const;
    bool GetBoolean(const std::string &name) const;
    CompoundTag *GetCompound(const std::string &name) const;
    ListTag *GetList(const std::string &name) const;

    void Remove(const std::string &name);
    void Clear();
    size_t Size() const;
    bool IsEmpty() const;
    const std::map<std::string, std::unique_ptr<Tag>> &GetTags() const;

private:
    std::map<std::string, std::unique_ptr<Tag>> m_tags;
};
