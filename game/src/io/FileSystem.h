#pragma once

#include "File.h"

class FileSystem
{
public:
    static File GetWorkingDirectory();
    static File GetExecutableDirectory();
    static File GetUserDirectory();
};
