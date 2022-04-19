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
    if (bytes) {
        LOGI("bytes length is %d", (int)bytes->length);
    }

    switch (flag) {
        case 1: {
            string filename = "iprhook/queststart" + currentDateTime() + ".bin";
            writeByte2File(filename.c_str(), bytes->buf, bytes->length);
            flag = 0;
            break;
        }
        case 2: {
            string filename = "iprhook/userClientGetAsync" + currentDateTime() + ".bin";
            writeByte2File(filename.c_str(), bytes->buf, bytes->length);
            flag = 0;
            break;
        }
        case 3: {
            string filename = "iprhook/tourLive" + currentDateTime() + ".bin";
            writeByte2File(filename.c_str(), bytes->buf, bytes->length);
            flag = 0;
            break;
        }
        case 4: {
            string filename = "iprhook/pvp" + currentDateTime() + ".bin";
            writeByte2File(filename.c_str(), bytes->buf, bytes->length);
            flag = 0;
            break;
        }
        case 6: {
            string filename = "iprhook/masterGet" + currentDateTime() + ".bin";
            writeByte2File(filename.c_str(), bytes->buf, bytes->length);
            flag = 0;
            break;
        }
        case 7: {
            string filename = "iprhook/gvg" + currentDateTime() + ".bin";
            writeByte2File(filename.c_str(), bytes->buf, bytes->length);
            flag = 0;
            break;
        }
        case 101: {
            string filename = "iprhook/noticeList" + currentDateTime() + ".bin";
            writeByte2File(filename.c_str(), bytes->buf, bytes->length);
            flag = 0;
            break;
        }
        default: break;
    }

    // 原始调用
    cSharpByteArray* r = decryptBackup(self, bytes, offset, length, key, iv, method);
    return r;
}

cSharpByteArray* (*encryptBackup) (void* self, cSharpByteArray* bytes, cSharpByteArray* key, cSharpByteArray* iv, const MethodInfo *method) = nullptr;
cSharpByteArray* encrypt(void* self, cSharpByteArray* bytes, cSharpByteArray* key, cSharpByteArray* iv, const MethodInfo *method){
    if(encryptBackup == nullptr){
        LOGE("backup DOES NOT EXIST");
    }
    LOGI("====== Encrypt ======");
    if (bytes) {
        LOGI("bytes length is %d", (int)bytes->length);
    }

    switch (flag) {
        case 5: {
            string filename = "iprhook/plain_saveDeck" + currentDateTime() + ".bin";
            writeByte2File(filename.c_str(), bytes->buf, bytes->length);
            flag = 0;
            break;
        }
        default: break;
    }
    // 原始调用
    cSharpByteArray* r = encryptBackup(self, bytes, key, iv, method);

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

void (*tourAreaLiveRequestBackup) (void* self, void* method) = nullptr;
void tourAreaLiveRequest(void* self, void* method) {
    if(tourAreaLiveRequestBackup == nullptr){
        LOGE("backup DOES NOT EXIST");
    }
    LOGI("calling tourAreaLiveRequest");
    if (flag == 0) {
        flag = 3;
    }
    tourAreaLiveRequestBackup(self, method);
}

void (*pvpStartRequestBackup) (void* self, void* method) = nullptr;
void pvpStartRequest(void* self, void* method) {
    if(pvpStartRequestBackup == nullptr){
        LOGE("backup DOES NOT EXIST");
    }
    LOGI("calling pvpStartRequest");
    if (flag == 0) {
        flag = 4;
    }
    pvpStartRequestBackup(self, method);
}

void (*gvgStartRequestBackup) (void* self, void* method) = nullptr;
void gvgStartRequest(void* self, void* method) {
    if(gvgStartRequestBackup == nullptr){
        LOGE("backup DOES NOT EXIST");
    }
    LOGI("calling gvgStartRequest");
    if (flag == 0) {
        flag = 7;
    }
    gvgStartRequestBackup(self, method);
}

void* (*userClientGetAsyncBackup) (void* self, void* request, void* headers, void* deadline, void* cancellationToken, void* method) = nullptr;
void* userClientGetAsync(void* self, void* request, void* headers, void* deadline, void* cancellationToken, void* method) {
    if(userClientGetAsyncBackup == nullptr){
        LOGE("backup DOES NOT EXIST");
    }
    LOGI("calling userClientGetAsync");
    if (flag == 0) {
        flag = 2;
    }
    void* r = userClientGetAsyncBackup(self, request, headers, deadline, cancellationToken, method);
    return r;
}

void* (*deckClientSaveAsyncBackup) (void* self, void* request, void* headers, void* deadline, void* cancellationToken, void* method) = nullptr;
void* deckClientSaveAsync(void* self, void* request, void* headers, void* deadline, void* cancellationToken, void* method) {
    if(deckClientSaveAsyncBackup == nullptr){
        LOGE("backup DOES NOT EXIST");
    }
    LOGI("calling deckClientSaveAsync");
    if (flag == 0) {
        flag = 5;
    }
    void* r = deckClientSaveAsyncBackup(self, request, headers, deadline, cancellationToken, method);
    return r;
}

void* (*masterClientGetAsyncBackup) (void* self, void* request, void* headers, void* deadline, void* cancellationToken, void* method) = nullptr;
void* masterClientGetAsync(void* self, void* request, void* headers, void* deadline, void* cancellationToken, void* method) {
    if(masterClientGetAsyncBackup == nullptr){
        LOGE("backup DOES NOT EXIST");
    }
    LOGI("calling masterClientGetAsync");
    if (flag == 0) {
        flag = 6;
    }
    void* r = masterClientGetAsyncBackup(self, request, headers, deadline, cancellationToken, method);
    return r;
}

void* (*noticeFetchAsyncBackup) (void* self, void* request, void* headers, void* deadline, void* cancellationToken, void* method) = nullptr;
void* noticeFetchAsync(void* self, void* request, void* headers, void* deadline, void* cancellationToken, void* method) {
    if(noticeFetchAsyncBackup == nullptr){
        LOGE("backup DOES NOT EXIST");
    }
    LOGI("calling noticeListAsync");
    if (flag == 0) {
        flag = 101;
    }
    void* r = noticeFetchAsyncBackup(self, request, headers, deadline, cancellationToken, method);
    return r;
}

void hackMain(const Il2CppAssembly** assembly_list, unsigned long size) {
    hackOne(assembly_list,
            size,
            "quaunity-api.Runtime",
            "Qua.Network",
            "DefaultMarshallerFactory",
            "Decrypt",
            -1,
            (void *) decrypt,
            (void **) &decryptBackup);

    hackOne(assembly_list,
            size,
            "quaunity-api.Runtime",
            "Qua.Network",
            "DefaultMarshallerFactory",
            "Encrypt",
            -1,
            (void *) encrypt,
            (void **) &encryptBackup);

    hackOne(assembly_list,
            size,
            "Assembly-CSharp",
            "Solis.Common.Proto.Api",
            "QuestStartRequest",
            ".ctor",
            -1,
            (void *) questStartRequest,
            (void **) &questStartRequestBackup);

    hackOne(assembly_list,
            size,
            "Assembly-CSharp",
            "Solis.Common.Proto.Api",
            "TourAreaLiveRequest",
            ".ctor",
            -1,
            (void *) tourAreaLiveRequest,
            (void **) &tourAreaLiveRequestBackup);

    hackOne(assembly_list,
            size,
            "Assembly-CSharp",
            "Solis.Common.Proto.Api",
            "PvpStartRequest",
            ".ctor",
            -1,
            (void *) pvpStartRequest,
            (void **) &pvpStartRequestBackup);

    hackOne(assembly_list,
            size,
            "Assembly-CSharp",
            "Solis.Common.Proto.Api",
            "GvgStartRequest",
            ".ctor",
            -1,
            (void *) gvgStartRequest,
            (void **) &gvgStartRequestBackup);

    hackOneNested(assembly_list,
                  size,
                  "Assembly-CSharp",
                  "Solis.Common.Proto.Api",
                  "User",
                  "UserClient",
                  "GetAsync",
                  4,
                  (void *) userClientGetAsync,
                  (void **) &userClientGetAsyncBackup);

    hackOneNested(assembly_list,
                  size,
                  "Assembly-CSharp",
                  "Solis.Common.Proto.Api",
                  "Deck",
                  "DeckClient",
                  "SaveAsync",
                  4,
                  (void *) deckClientSaveAsync,
                  (void **) &deckClientSaveAsyncBackup);

    hackOneNested(assembly_list,
                  size,
                  "Assembly-CSharp",
                  "Solis.Common.Proto.Api",
                  "Master",
                  "MasterClient",
                  "GetAsync",
                  4,
                  (void *) masterClientGetAsync,
                  (void **) &masterClientGetAsyncBackup);

//    hackOneNested(assembly_list,
//                  size,
//                  "Assembly-CSharp",
//                  "Solis.Common.Proto.Api",
//                  "Notice",
//                  "NoticeClient",
//                  "ListAsync",
//                  4,
//                  (void *) noticeFetchAsync,
//                  (void **) &noticeFetchAsyncBackup);
}
