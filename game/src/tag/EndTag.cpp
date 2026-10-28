#include "EndTag.h"

EndTag::EndTag()
{
}

Tag::eTAG_TYPE EndTag::GetType() const
{
    return eTagType_End;
}

void EndTag::Read(DataInputStream *inputStream)
{
    (void) inputStream;
}

void EndTag::Write(DataOutputStream *outputStream) const
{
    (void) outputStream;
}

std::unique_ptr<Tag> EndTag::Clone() const
{
    std::unique_ptr<EndTag> tag(new EndTag());
    tag->SetName(GetName());
    return tag;
}
