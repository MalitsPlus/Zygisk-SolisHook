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

cSharpByteArray* (*decryptBackup) (void* self, cSharpByteArray* bytes, int32_t offset, int32_t length, cSharpByteArray* key, cSharpByteArray* iv, const MethodInfo *method) = nullptr;
cSharpByteArray* decrypt(void* self, cSharpByteArray* bytes, int32_t offset, int32_t length, cSharpByteArray* key, cSharpByteArray* iv, const MethodInfo *method){
    if(decryptBackup == nullptr){
        LOGE("backup DOES NOT EXIST");
    }
    LOGI("====== Decrypt ======");
    if (key) {
        char *keyStr = getByteString(key->buf, key->length);
        LOGI("key is %s", keyStr);
    }

    writeByte2File("dec_keys.bin", bytes->buf, bytes->length);

    if (iv) {
        char *ivStr = getByteString(iv->buf, iv->length);
        LOGI("iv is %s", ivStr);
    }
    LOGI("offset(header length) is %d", offset);
    LOGI("message length is %d", length);

    // 原始调用
    cSharpByteArray* r = decryptBackup(self, bytes, offset, length, key, iv, method);
    return r;
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
}
