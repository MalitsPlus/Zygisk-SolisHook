#include <jni.h>
#include <sys/types.h>
#include <pthread.h>
#include <cstring>
#include <riru.h>
#include "include/config.h"
#include "hook_main.h"

/// 在从 Zygote fork 进程之前调用
static void forkAndSpecializePre(
        JNIEnv *env, jclass clazz, jint *uid, jint *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jint *mountExternal, jstring *seInfo, jstring *niceName,
        jintArray *fdsToClose, jintArray *fdsToIgnore, jboolean *is_child_zygote,
        jstring *instructionSet, jstring *appDataDir, jboolean *isTopApp, jobjectArray *pkgDataInfoList,
        jobjectArray *whitelistedDataInfoList, jboolean *bindMountAppDataDirs, jboolean *bindMountAppStorageDirs) {
    // Called "before" com_android_internal_os_Zygote_nativeForkAndSpecialize in frameworks/base/core/jni/com_android_internal_os_Zygote.cpp
    // Parameters are pointers, you can change the value of them if you want
    // Some parameters are not exist is older Android versions, in this case, they are null or 0

    // 检测当前进程是否为目标进程，通过路径+包名检测。包名可在 hook_main.h 中修改
    // 如果是则设置 enable_hack 标志
    // 此处仅负责设置标志，不负责其他任何业务逻辑处理
    enable_hack = isGame(env, *appDataDir);
}

static void forkAndSpecializePost(JNIEnv *env, jclass clazz, jint res) {
    // Called "after" com_android_internal_os_Zygote_nativeForkAndSpecialize in frameworks/base/core/jni/com_android_internal_os_Zygote.cpp
    // "res" is the return value of com_android_internal_os_Zygote_nativeForkAndSpecialize
    if (res == 0) {
        // in app process

        // When unload allowed is true, the module will be unloaded (dlclose) by Riru
        // If this modules has hooks installed, DONOT set it to true, or there will be SIGSEGV
        // This value will be automatically reset to false before the "pre" function is called
        riru_set_unload_allowed(!enable_hack);
        if (enable_hack) {
            int ret;
            pthread_t ntid;
            // 在 hack_thread 中实现 hook 逻辑
            if ((ret = pthread_create(&ntid, nullptr, hack_thread, nullptr))) {
                LOGE("can't create thread: %s\n", strerror(ret));
            }
        }
    } else {
        // in zygote process, res is child pid
        // don't print log here, see https://github.com/RikkaApps/Riru/blob/77adfd6a4a6a81bfd20569c910bc4854f2f84f5e/riru-core/jni/main/jni_native_method.cpp#L55-L66
    }
}

static void specializeAppProcessPre(
        JNIEnv *env, jclass clazz, jint *uid, jint *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jint *mountExternal, jstring *seInfo, jstring *niceName,
        jboolean *startChildZygote, jstring *instructionSet, jstring *appDataDir,
        jboolean *isTopApp, jobjectArray *pkgDataInfoList, jobjectArray *whitelistedDataInfoList,
        jboolean *bindMountAppDataDirs, jboolean *bindMountAppStorageDirs) {
    // Called "before" com_android_internal_os_Zygote_nativeSpecializeAppProcess in frameworks/base/core/jni/com_android_internal_os_Zygote.cpp
    // Parameters are pointers, you can change the value of them if you want
    // Some parameters are not exist is older Android versions, in this case, they are null or 0
    enable_hack = isGame(env, *appDataDir);
}

static void specializeAppProcessPost(
        JNIEnv *env, jclass clazz) {
    // Called "after" com_android_internal_os_Zygote_nativeSpecializeAppProcess in frameworks/base/core/jni/com_android_internal_os_Zygote.cpp

    // When unload allowed is true, the module will be unloaded (dlclose) by Riru
    // If this modules has hooks installed, DONOT set it to true, or there will be SIGSEGV
    // This value will be automatically reset to false before the "pre" function is called
    riru_set_unload_allowed(!enable_hack);
    if (enable_hack) {
        int ret;
        pthread_t ntid;
        if ((ret = pthread_create(&ntid, nullptr, hack_thread, nullptr))) {
            LOGE("can't create thread: %s\n", strerror(ret));
        }
    }
}

static void forkSystemServerPre(
        JNIEnv *env, jclass clazz, uid_t *uid, gid_t *gid, jintArray *gids, jint *runtimeFlags,
        jobjectArray *rlimits, jlong *permittedCapabilities, jlong *effectiveCapabilities) {
    // Called "before" com_android_internal_os_Zygote_forkSystemServer in frameworks/base/core/jni/com_android_internal_os_Zygote.cpp
    // Parameters are pointers, you can change the value of them if you want
    // Some parameters are not exist is older Android versions, in this case, they are null or 0

}

static void forkSystemServerPost(JNIEnv *env, jclass clazz, jint res) {
    // Called "after" com_android_internal_os_Zygote_forkSystemServer in frameworks/base/core/jni/com_android_internal_os_Zygote.cpp

    if (res == 0) {
        // In system server process
    } else {
        // In zygote process
    }
}

int shouldSkipUid(int uid) {
    // by default, Riru only call module functions in "normal app processes" (10000 <= uid % 100000 <= 19999)
    // false = don't skip
    return false;
}

static void onModuleLoaded() {
    // Called when this library is loaded and "hidden" by Riru (see Riru's hide.cpp)

    // If you want to use threads, start them here rather than the constructors
    // __attribute__((constructor)) or constructors of static variables,
    // or the "hide" will cause SIGSEGV
}

extern "C" {

int riru_api_version;
const char *riru_magisk_module_path = nullptr;
int *riru_allow_unload = nullptr;

static auto module = RiruVersionedModuleInfo{
        .moduleApiVersion = riru::moduleApiVersion,
        .moduleInfo= RiruModuleInfo{
                .supportHide = true,
                .version = riru::moduleVersionCode,
                .versionName = riru::moduleVersionName,
                .onModuleLoaded = onModuleLoaded,
                .forkAndSpecializePre = forkAndSpecializePre,
                .forkAndSpecializePost = forkAndSpecializePost,
                .forkSystemServerPre = forkSystemServerPre,
                .forkSystemServerPost = forkSystemServerPost,
                .specializeAppProcessPre = specializeAppProcessPre,
                .specializeAppProcessPost = specializeAppProcessPost
        }
};

RiruVersionedModuleInfo *init(Riru *riru) {
    auto core_max_api_version = riru->riruApiVersion;
    riru_api_version = core_max_api_version <= riru::moduleApiVersion ? core_max_api_version : riru::moduleApiVersion;
    module.moduleApiVersion = riru_api_version;

    riru_magisk_module_path = strdup(riru->magiskModulePath);
    if (riru_api_version >= 25) {
        riru_allow_unload = riru->allowUnload;
    }
    return &module;
}
}