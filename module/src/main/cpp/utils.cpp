//
// Created by Vibbit on 2022/6/12.
//

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
#include <chrono>
#include <sstream>
#include <iomanip>

using namespace std;

string getCsString(cSharpString* csString) {
    string result;
    result.append(csString->buf, 0, csString->length);
    return result;
}

string getByteString(uint8_t* buf, int length) {
    stringstream ss;
    ss << hex << setfill('0');
    for (int i = 0; i < length; i++) {
        ss << setw(2)  << (int)*(buf + i);
    }
    return ss.str();
}

string getCsByteString(cSharpByteArray* csBytes) {
    return getByteString(csBytes->buf, (int)csBytes->length);
}

string currentDateTime() {
    using chrono::system_clock;

    system_clock::time_point now = system_clock::now();
    chrono::milliseconds ms = chrono::duration_cast<chrono::milliseconds>(
            now.time_since_epoch()
    );
    int ms_t = ms.count() % 1000;

    time_t now_t = system_clock::to_time_t(now);
    struct tm tstruct {};
    char buf[16];
    tstruct = *localtime(&now_t);
    strftime(buf, sizeof(buf), "%y%m%d%H%M%S", &tstruct);
    sprintf(buf, "%s%03d", buf, ms_t);
    return buf;
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

/// openmode = 0: overwrite; openmode = ios::app: append
int writeText2File(const char* filename, const char* buf, size_t length, const ios::openmode mode) {
    string path = "/sdcard/Download/";
    path.append(filename);
    fstream file(path, ios::out | mode);
    if (!file) {
        LOGE("Error opening file %s", path.c_str());
        return -1;
    }
    file.write(buf, length);
    file.close();
    LOGI("Writes %s done", path.c_str());
    return 0;
}

string readTextFile(const char* filename) {
//    string path = "/sdcard/Download/";
    string path = "/data/data/game.qualiarts.idolypride/files/";
    path.append(filename);
    LOGI("%s", path.c_str());
    fstream file(path, ios::in);
    if (!file) {
        LOGE("Cannot open file %s.", path.c_str());
        LOGE("%s", strerror(errno));
        return "";
    }
    LOGI("Read %s succeed.", path.c_str());
    stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    LOGI("Read to buffer, returning to code.");
    return buffer.str();
}

u16string wreadTextFile(const char* filename) {
    string sstr = readTextFile(filename);
    u16string wbuffer = u16string(sstr.begin(), sstr.end());
    return wbuffer;
}
