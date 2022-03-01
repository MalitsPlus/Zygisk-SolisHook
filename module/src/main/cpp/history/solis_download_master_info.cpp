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

char* getCsharpString(cSharpString* sharpString) {
    return getString(sharpString->buf, sharpString->length);
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
/// 被替换的 dlopen 函数，这里只用于寻找 "libil2cpp.so" 的句柄
void* dlopen_(const char* name,
              int flags,
        //const void* extinfo,
              const void* caller_addr){
    // 调用原函数获取句柄
    void* handle = dlopen_backup(name, flags, /*extinfo,*/ caller_addr);
    if(!il2cpp_handle){
        LOGI("dlopen: %s", name);
        // 如果是目标 so 库，则将句柄复制保存到自定义的变量中，方便后续使用
        if(strstr(name, "libil2cpp.so")){
            il2cpp_handle = handle;
            LOGI("Got il2cpp handle at %lx", (long)il2cpp_handle);
        }
    }
    return handle;
}

// FIXME: ReturnType Argsm
typedef void (*Hook)(void* self, cSharpString* masterName, cSharpString* saveFilePath, cSharpByteArray* cryptoKey, void* methodInfo, void* method);
Hook backup = nullptr;
// FIXME: ReturnType Args
void hook(void* self, cSharpString* masterName, cSharpString* saveFilePath, cSharpByteArray* cryptoKey, void* methodInfo, void* method){
    if(backup == nullptr){
        LOGE("backup DOES NOT EXIST");
    }
    LOGI("====== MAGIC SHOW BEGINS ======");

    LOGI("masterName is %s", getCsharpString(masterName));
    LOGI("saveFilePath is %s", getCsharpString(saveFilePath));
    LOGI("cryptoKey is %s", getByteString(cryptoKey->buf, cryptoKey->length));

    // 原始调用
    backup(self, masterName, saveFilePath, cryptoKey, methodInfo, method);
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
    // 这个symbol_name 是哪来的？？
    void* loader_dlopen = DobbySymbolResolver(nullptr, "__dl__Z9do_dlopenPKciPK17android_dlextinfoPKv");
    LOGI("loader_dlopen got");

    LOGI("start hook_each 'loader_dlopen'");
    hook_each((unsigned long)loader_dlopen, (void*)dlopen_, (void**)&dlopen_backup);
    LOGI("hook_each 'loader_dlopen' completed");

    LOGI("start find addr of module 'libil2cpp.so'");
    while (true)
    {
        // 获取 module 基址，找到则跳出循环
        base_addr = get_module_base("libil2cpp.so");
        if (base_addr != 0 && il2cpp_handle != nullptr) {
            break;
        }
    }
    LOGD("detect libil2cpp.so %lx, start sleep", base_addr);

    // 获取各个 symbol 的地址，并直接 cast 为可直接调用的函数指针
    // 此处调用的函数全部为 il2cpp 源码中的关键函数，全部可以从 il2cpp 源码中找到具体实现
    //
    // 理论上说应该可以配合 IDA 直接找到任意目标函数 symbol？为什么要通过 il2cpp 的原始函数寻找地址？
    //
    // 猜想：il2cpp 会对原始函数 symbol 进行不规则的重命名，如果直接 dlsym 得到的结果会不可靠，
    // 并且每次游戏更新时，由于 symbol 随机可能会改变，所以用 il2cpp_class_get_method_from_name
    // 来指定 C# 级的干净 symbol 名进行获取。

    il2cpp_domain_get_ il2cpp_domain_get = (il2cpp_domain_get_)dlsym(il2cpp_handle, "il2cpp_domain_get");
    il2cpp_domain_get_assemblies_ il2cpp_domain_get_assemblies = (il2cpp_domain_get_assemblies_)dlsym(il2cpp_handle, "il2cpp_domain_get_assemblies");
    il2cpp_assembly_get_image_ il2cpp_assembly_get_image = (il2cpp_assembly_get_image_)dlsym(il2cpp_handle, "il2cpp_assembly_get_image");
    il2cpp_class_from_name_ il2cpp_class_from_name = (il2cpp_class_from_name_)dlsym(il2cpp_handle, "il2cpp_class_from_name");
    il2cpp_class_get_method_from_name_ il2cpp_class_get_method_from_name = (il2cpp_class_get_method_from_name_)dlsym(il2cpp_handle, "il2cpp_class_get_method_from_name");

    il2cpp_image_get_class_count_ il2cpp_image_get_class_count = (il2cpp_image_get_class_count_)dlsym(il2cpp_handle, "il2cpp_image_get_class_count");
    il2cpp_image_get_class_ il2cpp_image_get_class = (il2cpp_image_get_class_)dlsym(il2cpp_handle, "il2cpp_image_get_class");
    il2cpp_class_get_methods_ il2cpp_class_get_methods = (il2cpp_class_get_methods_)dlsym(il2cpp_handle, "il2cpp_class_get_methods");
    il2cpp_class_get_nested_types_ il2cpp_class_get_nested_types = (il2cpp_class_get_nested_types_)dlsym(il2cpp_handle, "il2cpp_class_get_nested_types");

    sleep(2);
    LOGW("hack game begin");
    Il2CppDomain* domain = il2cpp_domain_get();
    unsigned long ass_len = 0;
    // 获取当前 domain 中的所有 assembly
    const Il2CppAssembly** assembly_list = il2cpp_domain_get_assemblies(domain, &ass_len);
    // 循环遍历当前 domain 中的 assembly_list，直到找到 Assembly-CSharp

    // FIXME: Assembly Name
    while(strcmp((*assembly_list)->aname.name, "quaunity-master-manager.Runtime") != 0){
        LOGD("Assembly name: %s", (*assembly_list)->aname.name);
        assembly_list++;
    }
    LOGW("Assembly name: %s", (*assembly_list)->aname.name);
    // 获取当前 Assembly（此时为"Assembly-CSharp"）中的 image
    const Il2CppImage* image = il2cpp_assembly_get_image(*assembly_list);
    LOGI("image got at %lx", (long)image);


//    // 打印该 assembly 下所有类名
//    Il2CppClass* clas;
//    size_t count = il2cpp_image_get_class_count(image);
//    LOGI("count class %ld", (long)count);
//    for(size_t i = 0; i < count; i++) {
//        clas = (Il2CppClass*)il2cpp_image_get_class(image, i);
//        if (strcmp(clas->name, "Serializer") == 0) {
//            LOGI("one class %s", clas->name);
//            LOGI("one class %s, ns is %s, at %lx", clas->name, clas->namespaze, (long)clas);
//            break;
//        }
//    }
//    Il2CppClass* clazz = clas;


    // 根据 Namespace, Classname 获取 Class
    // FIXME: NameSpace ClassName
    Il2CppClass* clazz = il2cpp_class_from_name(image, "Qua.Master", "DownloadMasterFileInfo");
    LOGI("clazz got at %lx", (long)clazz);
    Il2CppClass* final_clazz = clazz;

    // FIXME: is nested class
    bool flag = false;
    if (flag) {
        // 注意：clazz->nested_type_count 可能不是count，需要验证结构体正确性
        uint16_t nested_type_count = clazz->nested_type_count;
        LOGI("nested_type_count %d", nested_type_count);

        for (int i = 0; i < nested_type_count; ++i) {
            Il2CppClass *c = clazz->nestedTypes[i];
            const char *nst_name = c->name;
            LOGI("one nested class named %s", nst_name);
            // FIXME: nested class name
            if (strcmp(nst_name, "Serializer") == 0) {
                final_clazz = c;
                break;
            }
        }
    }

    // 获取 Class 中的指定 MethodInfo，取其中的 methodPointer 获取地址，并 hook
    // 关于 MethodInfo 的结构，可参照 il2cpp 源码 il2cpp-class-internals.h#341
    // 参数个数为 c# 中的个数
    // FIXME: MethodName ArgsNum
    MethodInfo* methodInfo = (MethodInfo*)il2cpp_class_get_method_from_name(final_clazz, ".ctor", 4);
    LOGI("methodInfo got at %lx", (long)methodInfo);

    unsigned long addr = (unsigned long)methodInfo->methodPointer;
    LOGI("method got at %lx", addr);

    LOGI("start hook target method...");
    hook_each(addr, (void*)hook, (void**)&backup);

    LOGD("hack game finish");
    return nullptr;
}