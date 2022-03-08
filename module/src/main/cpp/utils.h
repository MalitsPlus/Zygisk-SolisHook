#ifndef RIRU_SOLISHOOK_UTILS_H
#define RIRU_SOLISHOOK_UTILS_H

struct cSharpByteArray {
    size_t idkwhatisthis[3];
    size_t length;
    uint8_t buf[4096];
};

struct cSharpString {
    size_t address; // size_t 在 arm64 中占8字节
    size_t nothing; // 不知为何，在android中看内存有这段，而在pc中查看内存时无这段
    int length;
    char buf[4096];
};

char* getString(char* buf, int length);
char* getCsharpString(cSharpString* sharpString);
void write2File(const char* filename, char* buf, const char* mode);
char* readFromFile(const char* path);
char* getByteString(uint8_t* buf, size_t length);
int writeByte2File(const char* filename, uint8_t* buf, size_t length);
char* readFromFile(const char* path);

#endif //RIRU_SOLISHOOK_UTILS_H
