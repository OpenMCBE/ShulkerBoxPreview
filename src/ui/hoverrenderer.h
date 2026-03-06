class HoverRenderer {
public:
    void* vtable;
    uint8_t pad0[0x10 - 0x08];
    std::string mFilteredContent;
    uint8_t pad1[0x40 - 0x28];

    float mCursorX;
    float mCursorY;
    float mOffsetX;
    float mOffsetY;

    float mBoxWidth;
    float mBoxHeight;
};
