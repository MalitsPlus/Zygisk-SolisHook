#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <cerrno>
#include <sys/mman.h>
#include <dlfcn.h>
#include <dobby.h>
#include <string>
#include <ios>
#include <sstream>
#include "hook_main.h"
#include "il2cpp.h"
#include <iomanip>
#include <sys/system_properties.h>
#include "utils.h"
#include "hook_overwrite.h"

using namespace std;

int flag = 0;

cSharpByteArray* (*decryptBackup) (void* self, cSharpByteArray* bytes, int32_t offset, int32_t length, cSharpByteArray* key, cSharpByteArray* iv, const MethodInfo *method) = nullptr;
cSharpByteArray* decrypt(void* self, cSharpByteArray* bytes, int32_t offset, int32_t length, cSharpByteArray* key, cSharpByteArray* iv, const MethodInfo *method){
    if(decryptBackup == nullptr){
        LOGE("backup DOES NOT EXIST");
    }
    LOGI("====== Decrypt ======");
    if (key) {
        string keyStr = getCsByteString(key);
        LOGI("key is %s", keyStr.c_str());
    }

    if (flag == 1) {
        string filename = "iprhook/queststart" + currentDateTime() + ".bin";
        writeByte2File(filename.c_str(), bytes->buf, bytes->length);
    }
    flag = 0;

    LOGI("bytes length is %d", (int)bytes->length);

    if (iv) {
        string ivStr = getCsByteString(iv);
        LOGI("iv is %s", ivStr.c_str());
    }

    LOGI("offset(header length) is %d", offset);
    LOGI("message length is %d", length);

    // 原始调用
    cSharpByteArray* r = decryptBackup(self, bytes, offset, length, key, iv, method);
    return r;
}

void (*questStartRequestBackup) (void* self, void* method) = nullptr;
void questStartRequest(void* self, void* method) {
    if(questStartRequestBackup == nullptr){
        LOGE("backup DOES NOT EXIST");
    }
    LOGI("calling questStartRequest");
    if (flag == 0) {
        flag = 1;
    }
    questStartRequestBackup(self, method);
}

void hackMain(const Il2CppAssembly** assembly_list) {
    hackOne(assembly_list,
            "quaunity-api.Runtime",
            "Qua.Network",
            "DefaultMarshallerFactory",
            "Decrypt",
            -1,
            (void*)decrypt,
            (void**)&decryptBackup);

    hackOne(assembly_list,
            "Assembly-CSharp",
            "Solis.Common.Proto.Api",
            "QuestStartRequest",
            ".ctor",
            -1,
            (void*)questStartRequest,
            (void**)&questStartRequestBackup);
}
