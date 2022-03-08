#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <cerrno>
#include <sys/mman.h>
#include <dlfcn.h>
#include <string>
#include <ios>
#include "utils.h"
#include "hook_main.h"
#include <iostream>
#include <fstream>

using namespace std;

string getCsString(cSharpString* csString) {
    string result;
    result.append(csString->buf, 0, csString->length);
    return result;
}

/// 将 char* 转为可以直接使用的 char*
char* getString(char* buf, int length) {
    // 由于 c++ 必须在字符串最后添加\0，故需要+1
    char* str = (char*)malloc(length + 1);
    memset(str, 0x00, length + 1);
    // 每个字符占2字节
    for (int i = 0; i < length; i++) {
        strcpy(str + i, buf + i * 2);
    }
    return str;
}

/// 获取结构体 cSharpString 中的 char*
char* getCsharpString(cSharpString* sharpString) {
    return getString(sharpString->buf, sharpString->length);
}

/// 以指定模式写文件
/// a+ or w+
void write2File(const char* filename, char* buf, const char* mode) {
    FILE *fp = nullptr;
    char path[256];
    memset(path, 0x00, 256);
    sprintf(path, "/sdcard/Download/%s", filename);

    fp = fopen(path, mode);

    if (fp) {
        LOGI("File opened: %s", path);
        // 在buf结尾处添加\n
        size_t length = strlen(buf);
        char buf2[length + 1];
        memset(buf2, 0x00, length + 1);
        sprintf(buf2, "%s\n", buf);
        LOGI("buf is %s", buf2);
        fputs(buf2, fp);
        fclose(fp);
        LOGI("Write file completed.");
    } else {
        LOGE("Error: [%d] %s", errno, strerror(errno));
    }
}

int writeByte2File(const char* filename, uint8_t* buf, size_t length) {
    string path = "/sdcard/Download/";
    path.append(filename);
    fstream file(path, ios::out | ios::binary);
    if (!file) {
        LOGE("Error opening file %s", path.c_str());
        return -1;
    }
    LOGI("Start writing file %s", path.c_str());
    file.write(reinterpret_cast<char *>(buf), length);
    file.close ();
    LOGI("Writes %s done", path.c_str());
    return 0;
}

int writeText2File(const char* filename, const char* buf, size_t length) {
    string path = "/sdcard/Download/";
    path.append(filename);
    fstream file(path, ios::out);
    if (!file) {
        LOGE("Error opening file %s", path.c_str());
        return -1;
    }
    file << buf;
    file.close();
    return 0;
}

/// 读取文本文件到字符串
char* readFromFile(const char* path) {
    FILE *fp = nullptr;
    fp = fopen(path, "r");
    char* buf;

    if (fp) {
        LOGI("Open file fp at %lx", (long)fp);
        fseek(fp, 0, SEEK_END);
        long size = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        LOGI("File size is %ld", size);

        buf = (char*)malloc(size);
        memset(buf, 0x00, size);

        char c;
        int i = 0;
        while (1) {
            c = fgetc(fp);
            if (feof(fp)) {
                break;
            }
            *(buf + i++) = c;
        }
        fclose(fp);
    } else {
        LOGE("Error: [%d] %s", errno, strerror(errno));
    }
    LOGI("Read file completed.");
    return buf;
}

/// 将byte转为char*
char* getByteString(uint8_t* buf, size_t length) {
    char* str = (char*)malloc(length * 2 + 1);
    memset(str, 0x00, length * 2 + 1);
    unsigned short tmp[4096];
    for (int i = 0; i < length; i++) {
        tmp[i] = buf[i];
    }
    for (int i = 0; i < length; i++) {
        sprintf(str + i * 2, "%02hx", tmp[i]);
    }
    return str;
}
