#ifndef IL2CPP_H
#define IL2CPP_H

#ifndef ALIGN_OF // Baselib header can also define this - if so use their definition.
#if defined(__GNUC__) || defined(__SNC__) || defined(__clang__)
#define ALIGN_OF(T) __alignof__(T)
#define ALIGN_TYPE(val) __attribute__((aligned(val)))
#define ALIGN_FIELD(val) ALIGN_TYPE(val)
#define IL2CPP_FORCE_INLINE inline __attribute__ ((always_inline))
#define IL2CPP_MANAGED_FORCE_INLINE IL2CPP_FORCE_INLINE
#elif defined(_MSC_VER)
#define ALIGN_OF(T) __alignof(T)
#if _MSC_VER >= 1900 && defined(__cplusplus)
    #define ALIGN_TYPE(val) alignas(val)
#else
    #define ALIGN_TYPE(val) __declspec(align(val))
#endif
    #define ALIGN_FIELD(val) __declspec(align(val))
    #define IL2CPP_FORCE_INLINE __forceinline
    #define IL2CPP_MANAGED_FORCE_INLINE inline
#else
    #define ALIGN_TYPE(size)
    #define ALIGN_FIELD(size)
    #define IL2CPP_FORCE_INLINE inline
    #define IL2CPP_MANAGED_FORCE_INLINE IL2CPP_FORCE_INLINE
#endif
#endif

#if defined(_MSC_VER)
#define IL2CPP_ZERO_LEN_ARRAY 0
#else
#define IL2CPP_ZERO_LEN_ARRAY 0
#endif

typedef void Il2CppDomain;
//typedef void Il2CppImage;
typedef struct Il2CppClass Il2CppClass;
typedef struct MethodInfo MethodInfo;

struct Il2CppImage {
    const char* name;
};


#define PUBLIC_KEY_BYTE_LENGTH 8
struct Il2CppAssemblyName
{
    const char* name;
    const char* culture;
    const char* hash_value;
    const char* public_key;
    uint32_t hash_alg;
    int32_t hash_len;
    uint32_t flags;
    int32_t major;
    int32_t minor;
    int32_t build;
    int32_t revision;
    uint8_t public_key_token[PUBLIC_KEY_BYTE_LENGTH];
};
struct Il2CppAssembly
{
    Il2CppImage* image;
    uint32_t token;
    int32_t referencedAssemblyStart;
    int32_t referencedAssemblyCount;
    Il2CppAssemblyName aname;
};
typedef void Il2CppDomain;

typedef struct VirtualInvokeData
{
    void* methodPtr;
    const MethodInfo* method;
} VirtualInvokeData;

typedef struct Il2CppClass {
    const Il2CppImage* image;
    void* gc_desc;
    const char* name;
    const char* namespaze;
    uint32_t byval_arg[4];
    uint32_t this_arg[4];
    Il2CppClass* element_class;
    Il2CppClass* castClass;
    Il2CppClass* declaringType;
    Il2CppClass* parent;
    size_t *generic_class;
    size_t *typeMetadataHandle;
    const size_t* interopData;
    Il2CppClass* klass;

    size_t* fields;
    const size_t* events; // Initialized in SetupEvents
    const size_t* properties; // Initialized in SetupProperties
    const MethodInfo** methods; // Initialized in SetupMethods
    Il2CppClass** nestedTypes; // Initialized in SetupNestedTypes
    Il2CppClass** implementedInterfaces; // Initialized in SetupInterfaces
    size_t* interfaceOffsets; // Initialized in Init
    void* static_fields; // Initialized in Init
    const size_t* rgctx_data; // Initialized in Init
    // used for fast parent checks
    Il2CppClass** typeHierarchy; // Initialized in SetupTypeHierachy

    void *unity_user_data;

    uint32_t initializationExceptionGCHandle;

    uint32_t cctor_started;
    uint32_t cctor_finished;
    ALIGN_TYPE(8) size_t cctor_thread;

    // Remaining fields are always valid except where noted
    size_t* genericContainerHandle;
    uint32_t instance_size; // valid when size_inited is true
    uint32_t actualSize;
    uint32_t element_size;
    int32_t native_size;
    uint32_t static_fields_size;
    uint32_t thread_static_fields_size;
    int32_t thread_static_fields_offset;
    uint32_t flags;
    uint32_t token;

    uint16_t method_count; // lazily calculated for arrays, i.e. when rank > 0
    uint16_t property_count;
    uint16_t field_count;
    uint16_t event_count;
    uint16_t nested_type_count;
    uint16_t vtable_count; // lazily calculated for arrays, i.e. when rank > 0
    uint16_t interfaces_count;
    uint16_t interface_offsets_count; // lazily calculated for arrays, i.e. when rank > 0

    uint8_t typeHierarchyDepth; // Initialized in SetupTypeHierachy
    uint8_t genericRecursionDepth;
    uint8_t rank;
    uint8_t minimumAlignment; // Alignment of this type
    uint8_t naturalAligment; // Alignment of this type without accounting for packing
    uint8_t packingSize;

    // this is critical for performance of Class::InitFromCodegen. Equals to initialized && !has_initialization_error at all times.
    // Use Class::UpdateInitializedAndNoError to update
    uint8_t initialized_and_no_error : 1;

    uint8_t valuetype : 1;
    uint8_t initialized : 1;
    uint8_t enumtype : 1;
    uint8_t is_generic : 1;
    uint8_t has_references : 1; // valid when size_inited is true
    uint8_t init_pending : 1;
    uint8_t size_init_pending : 1;
    uint8_t size_inited : 1;
    uint8_t has_finalize : 1;
    uint8_t has_cctor : 1;
    uint8_t is_blittable : 1;
    uint8_t is_import_or_windows_runtime : 1;
    uint8_t is_vtable_initialized : 1;
    uint8_t has_initialization_error : 1;
    VirtualInvokeData vtable[IL2CPP_ZERO_LEN_ARRAY];
} Il2CppClass;

typedef struct MethodInfo {
    void* methodPointer;
    void* invoker_method;
    const char* name;
    Il2CppClass *klass;
} MethodInfo;

// defied in grpc\src\core\tsi\transport_security_interface.h
typedef struct tsi_peer_property {
    char* name;
    struct {
        char* data;
        size_t length;
    } value;
} tsi_peer_property;

struct tsi_peer {
    tsi_peer_property* properties;
    size_t property_count;
};

typedef Il2CppDomain* (*il2cpp_domain_get_)();
typedef const Il2CppAssembly** (*il2cpp_domain_get_assemblies_) (const Il2CppDomain * domain, unsigned long * size);
typedef const Il2CppImage* (*il2cpp_assembly_get_image_) (const Il2CppAssembly * assembly);
typedef Il2CppClass* (*il2cpp_class_from_name_) (const Il2CppImage * image, const char* namespaze, const char *name);
typedef const MethodInfo* (*il2cpp_class_get_method_from_name_) (Il2CppClass * klass, const char* name, int argsCount);

typedef size_t (*il2cpp_image_get_class_count_) (const Il2CppImage * image);
typedef const Il2CppClass* (*il2cpp_image_get_class_) (const Il2CppImage * image, size_t index);
typedef const MethodInfo* (*il2cpp_class_get_methods_) (Il2CppClass *klass, void* *iter);
typedef Il2CppClass* (*il2cpp_class_get_nested_types_) (Il2CppClass *klass, void* *iter);

typedef int (*ssl_check_peer_) (const char* peer_name, const tsi_peer* peer, void* auth_context);

#endif