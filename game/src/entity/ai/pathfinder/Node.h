#pragma once

struct Node
{
    Node(int _x, int _y, int _z);

    float DistanceTo(const Node *other) const;

    static int CreateHash(int x, int y, int z);

    int x;
    int y;
    int z;
    float g;
    float h;
    float f;
    Node *cameFrom;
    bool open;
    bool closed;
};
