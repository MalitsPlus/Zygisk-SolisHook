//
// Created by kotori0 on 2020/2/5.
//

/*
 * dlopen: 打开一个库，获取句柄
 * dlsym: 在打开的库中查找符号的值，返回地址
 */

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

/// 判断当前的进程路径是否是目标游戏
int isGame(JNIEnv *env, jstring appDataDir) {
    if (!appDataDir)
        return 0;

    const char *app_data_dir = env->GetStringUTFChars(appDataDir, nullptr);

    int user = 0;
    static char package_name[256];
    if (sscanf(app_data_dir, "/data/%*[^/]/%d/%s", &user, package_name) != 2) {
        if (sscanf(app_data_dir, "/data/%*[^/]/%s", package_name) != 1) {
            package_name[0] = '\0';
            LOGW("can't parse %s", app_data_dir);
            return 0;
        }
    }
    env->ReleaseStringUTFChars(appDataDir, app_data_dir);
    if (strcmp(package_name, game_name) == 0) {
        LOGD("detect game: %s", package_name);
        return 1;
    }
    else {
        return 0;
    }
}

/// 获取 module_name 在内存中的基址
unsigned long get_module_base(const char* module_name)
{
    FILE *fp;
    unsigned long addr = 0;
    char *pch;
    char filename[32];
    char line[1024];

    // 为 filename 赋值
    snprintf(filename, sizeof(filename), "/proc/self/maps");
    // 以只读模式打开 module-内存 map
    fp = fopen(filename, "r");

    if (fp != nullptr) {
        // 读取 map 中每行，放入 line 中
        while (fgets(line, sizeof(line), fp)) {
            // 在 line 中检索是否存在 module_name 以及权限是否为 r-xp
            if (strstr(line, module_name) && strstr(line, "r-xp")) {
                // 以 "-" 为边界分割 line，返回值为分割的第一个字符串
                pch = strtok(line, "-");
                // 获取 pch 中的长整型(long)数字，即 module_name 的基址
                addr = strtoul(pch, nullptr, 16);
                // 为什么地址为 0x8000 时不算?
                // https://stackoverflow.com/questions/26237681/what-is-at-physical-memory-0x8000-32kb-to-0x10000-1mb-on-linux
                if (addr == 0x8000)
                    addr = 0;
                break;
            }
        }
        fclose(fp);
    }
    return addr;
}

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

/// 以w+模式写文件
void write2File(const char* filename, char* buf) {
    FILE *fp = nullptr;
    char path[256];
    memset(path, 0x00, 256);
    sprintf(path, "/sdcard/Download/%s", filename);

    fp = fopen(path, "w+");

    if (fp) {
        // 在buf结尾处添加\n
        size_t length = strlen(buf);
        char buf2[length + 1];
        memset(path, 0x00, length);
        sprintf(buf2, "%s\n", buf);
        fputs(buf2, fp);
        fclose(fp);
        LOGI("Write file completed.");
    } else {
        LOGE("Error: [%d] %s", errno, strerror(errno));
    }
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

typedef void* (*dlopen_type)(const char* name,
                             int flags,
        //const void* extinfo,
                             const void* caller_addr);
dlopen_type dlopen_backup = nullptr;
/// 被替换的 dlopen 函数，这里只用于寻找 "libgrpc_csharp_ext.so" 的句柄
void* dlopen_(const char* name,
              int flags,
        //const void* extinfo,
              const void* caller_addr){
    // 调用原函数获取句柄
    void* handle = dlopen_backup(name, flags, /*extinfo,*/ caller_addr);
    if(!il2cpp_handle){
        LOGI("dlopen: %s", name);
        // 如果是目标 so 库，则将句柄复制保存到自定义的变量中，方便后续使用
        if(strstr(name, "libgrpc_csharp_ext.so")){
            il2cpp_handle = handle;
            LOGI("dlopen got libgrpc_csharp_ext.so handle at %lx", (long)il2cpp_handle);
        }
    }
    return handle;
}

// FIXME: ReturnType Args
typedef void* (*Hook)(const char* pem_root_certs,
                      const char* key_cert_pair_cert_chain,
                      const char* key_cert_pair_private_key,
                      void* verify_peer_callback_tag);
Hook backup = nullptr;
// FIXME: ReturnType Args
void* hook(const char* pem_root_certs,
           const char* key_cert_pair_cert_chain,
           const char* key_cert_pair_private_key,
           void* verify_peer_callback_tag){
    if(backup == nullptr){
        LOGE("backup DOES NOT EXIST");
    }
    LOGI("====== MAGIC SHOW BEGINS ======");
    // 原始调用
    void* r = backup(pem_root_certs, key_cert_pair_cert_chain, key_cert_pair_private_key, verify_peer_callback_tag);

    LOGI("result is %lx", (long)r);
    LOGI("pem is is %s", pem_root_certs);
    LOGI("callback at %lx", (long)verify_peer_callback_tag);

    return r;
}


void hook_each(unsigned long rel_addr, void* hook, void** backup_){
    LOGI("hook_each: Installing hook at %lx", rel_addr);
    unsigned long addr = /*base_addr + */rel_addr;

    // 设置内存属性可写
    void* page_start = (void*)(addr - addr % PAGE_SIZE);
    if (-1 == mprotect(page_start, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_EXEC)) {
        LOGE("set mprotect failed(%d)", errno);
        return ;
    }
    LOGI("mprotect set successfully");

    DobbyHook(
            reinterpret_cast<void*>(addr),
            hook,
            backup_);

    // hook 完成后将内存属性修改回只读
    mprotect(page_start, PAGE_SIZE, PROT_READ | PROT_EXEC);
}

/// hook 逻辑入口点
void *hack_thread(void *arg)
{
    // tid
    LOGI("hack thread: %d", gettid());
    srand(time(nullptr));
    LOGI("start getting loader_dlopen");
    // libdl.so中的__dlopen函数只是将bin/linker中的dlopen函数包装了一下，实际逻辑都在linker中
    void* loader_dlopen = DobbySymbolResolver(nullptr, "__dl__Z9do_dlopenPKciPK17android_dlextinfoPKv");
    LOGI("loader_dlopen got at %lx", (long)loader_dlopen);

    LOGI("start hook_each 'loader_dlopen'");
    hook_each((unsigned long)loader_dlopen, (void*)dlopen_, (void**)&dlopen_backup);
    LOGI("hook_each 'loader_dlopen' completed");

    LOGI("start finding addr of module 'libgrpc_csharp_ext.so'");
    while (true)
    {
        // 获取 module 基址，找到则跳出循环
        base_addr = get_module_base("libgrpc_csharp_ext.so");
        if (base_addr != 0 && il2cpp_handle != nullptr) {
            break;
        }
    }
    LOGD("detect libgrpc_csharp_ext.so %lx, start sleep", base_addr);
    LOGD("handle at %lx", (long)il2cpp_handle);

    // 获取各个 symbol 的地址，并直接 cast 为可直接调用的函数指针
    // 此处调用的函数全部为 il2cpp 源码中的关键函数，全部可以从 il2cpp 源码中找到具体实现
    //
    // 理论上说应该可以配合 IDA 直接找到任意目标函数 symbol？为什么要通过 il2cpp 的原始函数寻找地址？
    //
    // 猜想：il2cpp 会对原始函数 symbol 进行不规则的重命名，如果直接 dlsym 得到的结果会不可靠，
    // 并且每次游戏更新时，由于 symbol 随机可能会改变，所以用 il2cpp_class_get_method_from_name
    // 来指定 C# 级的干净 symbol 名进行获取。

    auto grpcsharp_ssl_credentials_create = (ssl_check_peer_)dlsym(il2cpp_handle, "grpcsharp_ssl_credentials_create");
    LOGI("dlsym at %lx", (long)grpcsharp_ssl_credentials_create);
//    void* dobby_ssl_check_peer = DobbySymbolResolver(nullptr, "grpcsharp_ssl_credentials_create");
//    LOGD("hd at %lx", (long)dobby_ssl_check_peer);
    sleep(2);
    LOGW("hack game begin");

    LOGI("start hook target method...");
    hook_each((unsigned long)grpcsharp_ssl_credentials_create, (void*)hook, (void**)&backup);

    LOGD("hack game finish");

    return nullptr;
}