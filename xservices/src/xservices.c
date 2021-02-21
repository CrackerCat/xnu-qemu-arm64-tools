#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <libproc.h>
#include <execinfo.h>
#include <time.h>
#include <pthread.h>
#include <colors.h>
#include <dlfcn.h>
#include <xpc/xpc.h>
#include <fcntl.h>
#include <frida-gum.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOCFUnserialize.h>
#include <IOKit/IOKitLib.h>
#include <os/log.h>

//
// Interposing LibXPC - J Levin, http://NewOSXBook.com/
//
// Compile with gcc xpcsnoop -shared -o xpcsnoop.dylib
// 
// then shove forcefully with
//
// DYLD_INSERT_LIBRARIES=...
//
// Only the basic functionality, but the interesting one
// (i.e. snooping XPC messages!)
//
// Much more to be shown with MOXiI 2 Vol. I :-)
//
// License: Free to use and abuse, PROVIDED you :
//
// A) Give credit where credit is due 
// B) Don't remove this comment, especially if you GitHub this.
// C) Don't remove the const char *ver there, which is what what(1)
//    uses to identify binaries
// D) Don't complain this is "crap" or "shit" or any other vulgar word
//    or denigrating term - if you find a bug and/or have an improvement
//    submit it back to the source (j@ newosxbook) please.
// E) Spread good karma and/or help your fellow reversers and *OS enthusiasts
//

//
// This is the expected interpose structure
//typedef struct interpose_s {
//    void *new_func;
//    void *orig_func;
//} interpose_t;

// Our prototypes - requires since we are putting them in 
//  the interposing_functions, below

// Don't remove this!
const char *ver[] = { "@(#) PROGRAM: XPoCe	PROJECT: J's OpenXPC 1.0",
		      "@(#) http://www.NewOSXBook.com/ - Free for non-commercial use, but please give credit where due." };

typedef struct Hook_s {
    char *dylib_path;
    char *func_name;
    void *new_func;
    uint64_t func_offset;
} hook_t;

static int my_sleep(unsigned int seconds);
static xpc_type_t my_xpc_get_type(xpc_object_t obj);
static int64_t my_xpc_dictionary_get_int64(xpc_object_t dictionary,
                                           const char *key);
static const char *my_xpc_dictionary_get_string(xpc_object_t dictionary,
                                                const char *key);
static xpc_object_t my_xpc_connection_send_message_with_reply_sync(
                            xpc_connection_t connection, xpc_object_t message);
static void my_xpc_connection_send_message(xpc_connection_t connection,
                                           xpc_object_t message);
static void my_xpc_connection_send_message_with_reply(
                                        xpc_connection_t connection,
                                        xpc_object_t message,
                                        dispatch_queue_t targetq,
                                        xpc_handler_t handler);
static void *my_libxpc_initializer(void);
static int my_xpc_pipe_routine(void *xpcPipe, xpc_object_t *inDict,
                               xpc_object_t *outDict);
static xpc_connection_t my_xpc_connection_create(const char *name,
                                                 dispatch_queue_t targetq);

static int my_sleep(unsigned int seconds);

static kern_return_t my_IOServiceAddMatchingNotification(
                                            IONotificationPortRef notifyPort,
                                            const io_name_t notificationType,
                                            CFDictionaryRef matching,
                                            IOServiceMatchingCallback callback,
                                            void *refCon,
                                            io_iterator_t *notification);
static kern_return_t my_IOServiceAddNotification(mach_port_t masterPort,
                                              const io_name_t notificationType,
                                              CFDictionaryRef matching,
                                              mach_port_t wakePort,
                                              uintptr_t reference,
                                              io_iterator_t *notification);
static io_object_t my_IOIteratorNext(io_iterator_t iterator);
static kern_return_t my_IOServiceGetMatchingServices(mach_port_t masterPort,
                                                     CFDictionaryRef matching,
                                                     io_iterator_t *existing);
static io_service_t my_IOServiceGetMatchingService(mach_port_t masterPort,
                                                   CFDictionaryRef matching);
static CFMutableDictionaryRef my_IOServiceMatching(const char *name);
static kern_return_t my_IOServiceOpen(io_service_t service,
                                      task_port_t owningTask,
                                      uint32_t type, io_connect_t *connect);
static io_registry_entry_t my_IORegistryEntryFromPath(mach_port_t masterPort,
                                                      const io_string_t path);
static kern_return_t my_IOConnectCallMethod(mach_port_t connection,
                                            uint32_t selector,
                                            const uint64_t *input,
                                            uint32_t inputCnt,
                                            const void *inputStruct,
                                            size_t inputStructCnt,
                                            uint64_t *output,
                                            uint32_t *outputCnt,
                                            void *outputStruct,
                                            size_t *outputStructCnt);
static kern_return_t my_IOConnectCallScalarMethod(mach_port_t connection,
                                                  uint32_t selector,
                                                  const uint64_t *input,
                                                  uint32_t inputCnt,
                                                  uint64_t *output,
                                                  uint32_t *outputCnt);
kern_return_t my_IOConnectCallStructMethod(mach_port_t connection,
                                           uint32_t selector,
                                           const void *inputStruct,
                                           size_t inputStructCnt,
                                           void *outputStruct,
                                           size_t *outputStructCnt);
static kern_return_t my_IOConnectCallAsyncMethod(mach_port_t connection,
                                                 uint32_t selector,
                                                 mach_port_t wake_port,
                                                 uint64_t *reference,
                                                 uint32_t referenceCnt,
                                                 const uint64_t *input,
                                                 uint32_t inputCnt,
                                                 const void *inputStruct,
                                                 size_t inputStructCnt,
                                                 uint64_t *output,
                                                 uint32_t *outputCnt,
                                                 void *outputStruct,
                                                 size_t *outputStructCnt);
static kern_return_t my_IOConnectCallAsyncStructMethod(
                                                 mach_port_t connection,
                                                 uint32_t selector,
                                                 mach_port_t wake_port,
                                                 uint64_t *reference,
                                                 uint32_t referenceCnt,
                                                 const void *inputStruct,
                                                 size_t inputStructCnt,
                                                 void *outputStruct,
                                                 size_t *outputStructCnt);
static kern_return_t my_IOConnectCallAsyncScalarMethod(
                                                 mach_port_t connection,
                                                 uint32_t selector,
                                                 mach_port_t wake_port,
                                                 uint64_t *reference,
                                                 uint32_t referenceCnt,
                                                 const uint64_t *input,
                                                 uint32_t inputCnt,
                                                 uint64_t *output,
                                                 uint32_t *outputCnt);

static uint64_t my_MGCopyAnswer_internal(CFStringRef cfstr, uint32_t *type);

static hook_t hooks[] = {

 //{ "/usr/lib/system/libxpc.dylib", "xpc_get_type", &my_xpc_get_type, 0 },
 { "/usr/lib/system/libxpc.dylib", "xpc_dictionary_get_int64",
   &my_xpc_dictionary_get_int64, 0 },
 { "/usr/lib/system/libxpc.dylib", "xpc_dictionary_get_string",
   &my_xpc_dictionary_get_string, 0 },
 { "/usr/lib/system/libxpc.dylib",
   "xpc_connection_send_message_with_reply_sync",
   &my_xpc_connection_send_message_with_reply_sync, 0 },
 { "/usr/lib/system/libxpc.dylib", "xpc_connection_send_message",
   &my_xpc_connection_send_message, 0 },
 { "/usr/lib/system/libxpc.dylib", "xpc_connection_send_message_with_reply",
   &my_xpc_connection_send_message_with_reply, 0 },
 //{ "/usr/lib/system/libxpc.dylib", "_libxpc_initializer",
 //  &my_libxpc_initializer, 0 },
 { "/usr/lib/system/libxpc.dylib", "xpc_pipe_routine",
   &my_xpc_pipe_routine, 0 },
 { "/usr/lib/system/libxpc.dylib", "xpc_connection_create",
   &my_xpc_connection_create, 0 },

 { "/usr/lib/system/libsystem_c.dylib", "sleep", &my_sleep, 0 },

 { "/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit",
   "IOServiceAddMatchingNotification",
   &my_IOServiceAddMatchingNotification, 0 },
 { "/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit",
   "IOServiceAddNotification", &my_IOServiceAddNotification, 0 },
   //TODO: JOANTHANA too many results, consider filtering by backtrace frames
   //strstr() to search for "libMobileGestalt.dylib" which is the with the
   //noisy hits
 //{ "/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit",
 //  "IOIteratorNext", &my_IOIteratorNext, 0 },
 { "/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit",
   "IOServiceGetMatchingServices", &my_IOServiceGetMatchingServices, 0 },
 { "/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit",
   "IOServiceGetMatchingService", &my_IOServiceGetMatchingService, 0 },
 { "/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit",
   "IOServiceMatching", &my_IOServiceMatching, 0 },
 { "/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit",
   "IOServiceOpen", &my_IOServiceOpen, 0 },
 { "/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit",
   "IORegistryEntryFromPath", &my_IORegistryEntryFromPath, 0 },
 { "/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit",
   "IOConnectCallMethod", &my_IOConnectCallMethod, 0 },
 { "/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit",
   "IOConnectCallScalarMethod", &my_IOConnectCallScalarMethod, 0 },
 { "/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit",
   "IOConnectCallStructMethod", &my_IOConnectCallStructMethod, 0 },
 { "/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit",
   "IOConnectCallAsyncMethod", &my_IOConnectCallAsyncMethod, 0 },
 { "/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit",
   "IOConnectCallAsyncStructMethod", &my_IOConnectCallAsyncStructMethod, 0 },
 { "/System/Library/Frameworks/IOKit.framework/Versions/A/IOKit",
   "IOConnectCallAsyncScalarMethod", &my_IOConnectCallAsyncScalarMethod, 0 },

 { "/usr/lib/libMobileGestalt.dylib", "MGCopyAnswer",
   &my_MGCopyAnswer_internal, 8 },
 { NULL, NULL, NULL },
};

typedef struct {
    char *orig_str;
    char *str;
    size_t *bytes_left;
} ostr_t;

int g_backtrace = 0 ;
int g_hex = 0;
int g_myDesc = 1;
int g_color = 1;
int g_noIncoming = 0;
int g_fake_resp = 0;
int g_skip_xpc = 0;
int g_file_out = 0;
pthread_mutex_t outputMutex = PTHREAD_MUTEX_INITIALIZER;
int id = 1;

void *xpc_dictionary_get_connection(xpc_object_t dict);
int xpc_describe(xpc_object_t obj, int indent, ostr_t *ostr);
void xpc_dictionary_apply_f(xpc_object_t dictionary, void *context, void *f);

xpc_object_t processedDict = NULL;

FILE *outputf = NULL;

int g_indent = 0;
int g_reply  = 0; // unused

#define PRINT_STRING_LEN (32768)

//32MB
#define MAX_LOG_FSIZE (0x2000000)

#define APPLE_KEY_STORE_SRV_MATCH (1)
#define APPLE_KEY_STORE_CONNECTION (1)

static void add_string(ostr_t *ostr, char *format, ...)
{
    va_list args;
    if (0 == *ostr->bytes_left) {
        return;
    }
    va_start(args, format);
    size_t size = vsnprintf(ostr->str, *ostr->bytes_left, format, args);
    va_end(args);
    if (size >= *ostr->bytes_left) {
        *ostr->bytes_left = 0;
    } else {
        *ostr->bytes_left -= size;
        ostr->str += size;
    }
}

static void print_string(ostr_t *ostr)
{
    pthread_mutex_lock(&outputMutex);
    if (g_file_out && outputf) {
        //TODO: JONATHANA size limit seems to not work?
        off_t fsize = fseek(outputf, 0, SEEK_END);
        if (fsize >= MAX_LOG_FSIZE) {
            pthread_mutex_unlock(&outputMutex);
            return;
        }
    }
    if (g_file_out && outputf) {
        fprintf(outputf, "\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
        fprintf(outputf, "%s", &ostr->orig_str[0]);
    } else {
        os_log_error(OS_LOG_DEFAULT,
                     "\n<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
        os_log_error(OS_LOG_DEFAULT, "%{public}s", &ostr->orig_str[0]);
    }
    if ((0 == *ostr->bytes_left) || (1 == *ostr->bytes_left)) {
        if (g_file_out && outputf) {
            fprintf(outputf, "\nSTRING SIZE NOT ENOUGH!\n");
        } else {
            os_log_error(OS_LOG_DEFAULT, "\nSTRING SIZE NOT ENOUGH!\n");
        }
    }
    if (g_file_out && outputf) {
        fprintf(outputf, "\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
        fflush(outputf);
    } else {
        os_log_error(OS_LOG_DEFAULT,
                     "\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    }
    pthread_mutex_unlock(&outputMutex);
}

//TODO: JONATHANA
//static char *add_to_string(char *str, size_t *bytes_left, char *format, ...)
//{
//    va_list args;
//    if (0 == *bytes_left) {
//        return str;
//    }
//    va_start(args, format);
//    size_t size = snprintf(str, *bytes_left, format, args);
//    va_end(args);
//    if (size >= *bytes_left) {
//        *bytes_left = 0;
//        return str;
//    } else {
//        *bytes_left -= size;
//        return str + size;
//    }
//}

char *get_proc_name(int pid, char *buf, size_t size)
{
    // proc_info makes for much easier API than sysctl..
    if (!pid) return ("?");
    buf[0] = '\0';
    proc_name(pid, buf, size);
    return buf;
}

char *getTimeStamp(char *buf, size_t size)
{
    if (size < 256) {
        return "buffer too small for timestamp";
    }

    buf[0] = '\0';
    time_t now = time(NULL);
    ctime_r(&now, buf);
    // ctime returns \n\0 - we dont want the \n
    char *newline = strchr(buf, '\n');
    if (newline) *newline = '\0';
    return buf;
}

static void cf_dumper(CFTypeRef cfobj, uint64_t indent, ostr_t *ostr)
{
    for (uint64_t i = 0; i < indent; i++) {
        add_string(ostr, "    ");
    }
    if (CFStringGetTypeID() == CFGetTypeID(cfobj)) {
        CFIndex length = CFStringGetLength(cfobj);
        CFIndex maxSize =
          CFStringGetMaximumSizeForEncoding(length, kCFStringEncodingUTF8) + 1;
        char str[1024];
        if (maxSize >= 1024) {
            fprintf(stderr, "String: string too long\n");
            abort();
        }
        if (CFStringGetCString(cfobj, &str[0], 1024, kCFStringEncodingUTF8)) {
            add_string(ostr, "String: %s", str);
            return;
        } else {
            fprintf(stderr, "error in CFStringGetCString()\n");
            abort();
        }
        return;
    }
    if (CFDataGetTypeID() == CFGetTypeID(cfobj)) {
        add_string(ostr, "Data: TODO: implement me");
        return;
    }
    if (CFNumberGetTypeID() == CFGetTypeID(cfobj)) {
        //TODO: Warning - this will break if size is ever greater than 8
        uint64_t num = 0;
        CFNumberType numType = CFNumberGetType((CFNumberRef)cfobj);
        CFNumberGetValue((CFNumberRef)cfobj, numType, (void *)&num);
        add_string(ostr, "Number: 0x%016llx", num);
        return;
    }
    if (CFBooleanGetTypeID() == CFGetTypeID(cfobj)) {
        add_string(ostr, "Boolean: %d",
                   CFBooleanGetValue((CFBooleanRef)cfobj));
        return;
    }
    if (CFDateGetTypeID() == CFGetTypeID(cfobj)) {
        add_string(ostr, "Date: TODO: implement me");
        return;
    }
    if (CFDictionaryGetTypeID() == CFGetTypeID(cfobj)) {
        add_string(ostr, "Dictionary:\n");
        CFIndex count = CFDictionaryGetCount(cfobj);
        if (0 == count) {
            return;
        }
        CFTypeRef *keys, *values;
        CFIndex k;
        keys = (CFTypeRef *)CFAllocatorAllocate(kCFAllocatorDefault, 2 * count
                                                       * sizeof(CFTypeRef), 0);
        if (!keys) {
            fprintf(stderr, "error in CFAllocatorAllocate()\n");
            abort();
            return;
        }
        values = keys + count;
        CFDictionaryGetKeysAndValues(cfobj, keys, values);
        for (k = 0; k < count; k++) {
            if (!keys[k]) {
                fprintf(stderr, "error got null key\n");
                CFAllocatorDeallocate(kCFAllocatorDefault, keys);
                abort();
                return;
            }
            if (!values[k]) {
                fprintf(stderr, "error got null value\n");
                CFRelease(keys[k]);
                CFAllocatorDeallocate(kCFAllocatorDefault, keys);
                abort();
                return;
            }
            cf_dumper((CFTypeRef)keys[k], indent + 1, ostr);
            cf_dumper((CFTypeRef)values[k], indent + 2, ostr);
            CFRelease(keys[k]);
            CFRelease(values[k]);
        }
        CFAllocatorDeallocate(kCFAllocatorDefault, keys);
        return;
    }
    if (CFArrayGetTypeID() == CFGetTypeID(cfobj)) {
        add_string(ostr, "Array:\n");
        CFIndex k, c = CFArrayGetCount(cfobj);
        if (0 == c) {
            return;
        }
        CFTypeRef *values =
            (CFTypeRef *)CFAllocatorAllocate(kCFAllocatorDefault,
                                             c * sizeof(CFTypeRef), 0);
        if (!values) {
            fprintf(stderr, "error in CFAllocatorAllocate()\n");
            abort();
            return;
        }
        for (k = 0; k < c; k++) {
            if (!values[k]) {
                fprintf(stderr, "error got null value\n");
                CFAllocatorDeallocate(kCFAllocatorDefault, values);
                abort();
                return;
            }
            cf_dumper((CFTypeRef)values[k], indent + 1, ostr);
            CFRelease(values[k]);
        }
        CFAllocatorDeallocate(kCFAllocatorDefault, values);
        return;
    }
    if (CFSetGetTypeID() == CFGetTypeID(cfobj)) {
        add_string(ostr, "Set: TODO: implement me\n");
        return;
    }
    if (CFNullGetTypeID() == CFGetTypeID(cfobj)) {
        add_string(ostr, "Null: TODO: implement me\n");
        return;
    }
    fprintf(stderr, "error unknown cfobj type\n");
    abort();
    return;
}

static void print_data(ostr_t *ostr, uint8_t *data, size_t len)
{
    uint8_t *bytes = (uint8_t *)data;
    for (size_t i = 0; i < len; i++) {
        uint8_t c = bytes[i];
        if ((g_hex) || (c < 0x20) || (c > 0x7e)) {
           add_string(ostr, "\\x%02x", c);
        } else {
           if (*ostr->bytes_left > 1) {
               ostr->str[0] = c;
               ostr->str++;
               ostr->str[0] = '\0';
               (*ostr->bytes_left)--;
           }
        }
    }
}

int dictDumper(const char *key, xpc_object_t value, ostr_t *ostr)
{
    // Do iteration.
    int i = g_indent;
    while (i > 0) {
        add_string(ostr, "  ");
        i--;
    }
    if (0) {
        add_string(ostr, "Key: %s, Value: ", key);
    } else {
        add_string(ostr, "%s: ", key);
    }
    xpc_type_t type = xpc_get_type(value);
    if (type == XPC_TYPE_ACTIVITY) {
        add_string(ostr, "Activity...");
    }
    if (type == XPC_TYPE_DATE) {
        add_string(ostr, "DATE...");
    }
    if (type == XPC_TYPE_FD) { 
        char buf[4096];
        int fd = xpc_fd_dup(value);
        fcntl(fd, F_GETPATH, buf);
        add_string(ostr, "FD: %s", buf);
    }
    if (type == XPC_TYPE_UUID) { 
        char buf[256];
        uuid_unparse(xpc_uuid_get_bytes(value), buf);
        add_string(ostr, "UUID: %s", buf);
    }
    if (type == XPC_TYPE_SHMEM) {
        add_string(ostr, "Shared memory (not handled yet)...");
    }
    if (type == XPC_TYPE_ENDPOINT) {
        add_string(ostr, "XPC Endpoint");
    }
    if (type == XPC_TYPE_BOOL) { 
        add_string(ostr, xpc_bool_get_value(value) ? "true" : "false");
    }
    if (type == XPC_TYPE_DATA)
    {
        int len = xpc_data_get_length(value);
        add_string(ostr, "Data (%d bytes): ", len);
        const uint8_t *bytes = (const uint8_t *)xpc_data_get_bytes_ptr(value);
        // print with nulls
        if (bytes) {
            //TODO: JONATHTANA
            //if ((len >= strlen("bplist")) &&
            //    (0 == memcmp(&bytes[0], "bplist", strlen("bplist")))) {
            //    CFDataRef data =
            //        CFDataCreateWithBytesNoCopy(kCFAllocatorDefault,
            //                                    (uint8_t *)&bytes[0], len,
            //                                    kCFAllocatorNull);
            //    if (!data) {
            //        fprintf(output, "error CFDataCreateWithBytesNoCopy()\n");
            //        //abort();
            //        fprintf(output,"\n");
            //        return 1;
            //    }
            //    CFErrorRef error;
            //    CFPropertyListRef plist = (CFPropertyListRef)
            //            CFPropertyListCreateWithData(kCFAllocatorDefault,
            //                                   data, 0, NULL, &error);
            //    if (!plist) {
            //        fprintf(output, "error parsing plist\n");
            //        CFStringRef s = CFErrorCopyFailureReason(error);
            //        cf_dumper(s, 1);
            //        CFRelease(s);
            //        CFRelease(error);
            //        return 1;
            //        //abort();
            //    }
            //    CFRelease(error);
            //    CFRelease(data);
            //    cf_dumper((CFTypeRef)plist, 1);
            //    CFRelease(plist);

            //    //TODO: JONATHANA second try
            //    //CFStringRef error;
            //    //CFPropertyListRef plist = (CFPropertyListRef)
            //    //        IOCFUnserialize(&bytes[0],
            //    //                        kCFAllocatorDefault, /* options */ 0,
            //    //                        &error);
            //    //if (!plist) {
            //    //    fprintf(output, "error parsing plist\n");
            //    //    cf_dumper(error, 1);
            //    //    CFRelease(error);
            //    //    abort();
            //    //}
            //    //cf_dumper((CFTypeRef)plist, 1);
            //    //CFRelease(plist);
            //} else {
                print_data(ostr, (uint8_t *)bytes , len);
            //}
        } // bytes
    }
    if (xpc_get_type(value) == XPC_TYPE_ARRAY) {
        add_string(ostr, "// Array (%zu values)\n", xpc_array_get_count(value));
        int i = 0;
        for (i = 0; i < xpc_array_get_count(value); i++)
        {
           char elem[1024];
           sprintf(elem, "%s[%d]", key, i);
           void *arrayElem = xpc_array_get_value(value, i);
           dictDumper(elem, arrayElem, ostr);
        }
    }
    if (xpc_get_type(value) == XPC_TYPE_INT64) { 
        int64_t val = xpc_int64_get_value(value);
        add_string(ostr, "%c%lld", val >= 0 ? '+' : ' ',
                   xpc_int64_get_value(value));
    }
    if (xpc_get_type(value) == XPC_TYPE_UINT64) {
        add_string(ostr, "%lld", xpc_uint64_get_value(value));
    }
    if (xpc_get_type(value) == XPC_TYPE_DICTIONARY) { 
        add_string(ostr, "// (dictionary %p)", value);
        xpc_describe(value, g_indent + 1, ostr);
    }
    if (xpc_get_type (value) == XPC_TYPE_STRING) {
        add_string(ostr, "\"%s\"", xpc_string_get_string_ptr(value));
    }
    add_string(ostr, "\n");
    return 1;
}

int xpc_describe(xpc_object_t obj, int indent, ostr_t *ostr)
{
    char buf[1024];

    if (!g_myDesc) {
        add_string(ostr, "xpccd: %s\n", xpc_copy_description(obj));
        return 0;
    }
    add_string(ostr, "xpccd: %s\n", xpc_copy_description(obj));
    g_indent++;
    if (xpc_get_type (obj) == XPC_TYPE_CONNECTION) {
        int pid = xpc_connection_get_pid(obj);
        add_string(ostr, "Peer: %s, PID: %d (%s) \n",
                   xpc_connection_get_name(obj), pid,
                   get_proc_name(pid, buf, sizeof(buf)));
        return 0;
    }
    if (xpc_get_type(obj) == XPC_TYPE_ARRAY) {
        add_string(ostr, "Array (@TODO)\n");
        return (0);
    }
    if (xpc_get_type(obj) == XPC_TYPE_DICTIONARY) {
        add_string(ostr, "--- Dictionary %p, %zu values:\n",
                   obj, xpc_dictionary_get_count(obj));
        xpc_dictionary_apply_f(obj, ostr, dictDumper);
        g_indent--;
        add_string(ostr, "--- End Dictionary %p\n", obj);
        return 0;
    }
    return 1;
}

static inline void do_backtrace(ostr_t *ostr)
{
    static void *backtraceBuf[1024];
    int num = backtrace(backtraceBuf, 1024);
    char **backtraceSyms = backtrace_symbols(backtraceBuf, num);

    if (backtraceSyms) {
        int i = 0;
        for (i = 0; i < num; i++) {
            add_string(ostr, "Frame %i: %s\n", i,backtraceSyms[i]);
        }
        free(backtraceSyms);
    }
}

//TODO: JONATHANA can the fact that we block until reply break the
//      logic of the function
static void my_xpc_connection_send_message_with_reply(
                                        xpc_connection_t connection,
                                        xpc_object_t message,
                                        dispatch_queue_t targetq,
                                        xpc_handler_t handler)
{
    //asm volatile ("hlt #0");
    // On an outgoing message, we can clear processed Dict
    char buf[1024];

    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    processedDict = NULL;
    if (g_color) add_string(&ostr, RED);
    id++;
    add_string(&ostr, "XPCCSMWR %s (%d) Outgoing ==> ",
               getTimeStamp(buf, sizeof(buf)), id);
    g_indent = 0;
    xpc_describe(connection, 0, &ostr);
    add_string(&ostr, " queue: %s, \n", dispatch_queue_get_label(targetq));
    xpc_describe(message, 0, &ostr);
    if (g_color) add_string(&ostr, NORMAL);
    if (g_backtrace) do_backtrace(&ostr);
    xpc_object_t reply =
              xpc_connection_send_message_with_reply_sync(connection, message);
    if (g_color) add_string(&ostr, CYAN);
    add_string(&ostr, "%s <== (reply sync)\n", getTimeStamp(buf, sizeof(buf)));
    g_indent = 0;
    xpc_describe(reply, 0, &ostr);
    if (g_color) add_string(&ostr, NORMAL);
    if (!g_skip_xpc) {
        print_string(&ostr);
    }
    handler(reply);
}

static xpc_object_t my_xpc_connection_send_message_with_reply_sync(
                            xpc_connection_t connection, xpc_object_t message)
{
    //asm volatile ("hlt #0");
    // On an outgoing message, we can clear processed Dict
    char buf[1024];

    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    processedDict = NULL;
    xpc_object_t reply;
    if (g_color) add_string(&ostr, RED);
    add_string(&ostr, "XPCCSMWRS %s (%d) Outgoing ==> ",
               getTimeStamp(buf, sizeof(buf)), id);
    id++;
    xpc_describe(connection, 0, &ostr);
    add_string(&ostr, " (with reply sync)\n");
    xpc_describe(message, 1, &ostr);

    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    if (g_color) add_string(&ostr, CYAN);
    reply = xpc_connection_send_message_with_reply_sync(connection, message);
    add_string(&ostr, "%s <== ", getTimeStamp(buf, sizeof(buf)));
    g_indent = 0;
    xpc_describe(connection, 0, &ostr);
    add_string(&ostr, "\n");
    xpc_describe(reply, 1, &ostr);
    if (g_color) add_string(&ostr, NORMAL);
    if (!g_skip_xpc) {
        print_string(&ostr);
    }
    return (reply);

}

static void my_xpc_connection_send_message(xpc_connection_t connection,
                                           xpc_object_t message)
{
    //asm volatile ("hlt #0");
    char buf[1024];

    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    if (g_color) add_string(&ostr, RED);
    add_string(&ostr, "XPCCSM %s ==> ", getTimeStamp(buf, sizeof(buf)));
    g_indent = 0;
    xpc_describe(connection, 0, &ostr);
    add_string(&ostr, "\n");
    xpc_describe(message, 0, &ostr);
    if (g_color) add_string(&ostr, NORMAL);
    if (g_backtrace) do_backtrace(&ostr);
    if (!g_skip_xpc) {
        print_string(&ostr);
    }
    xpc_connection_send_message(connection, message);
}

extern int xpc_pipe_routine(void *xpcPipe, xpc_object_t *inDict,
                            xpc_object_t *out);
static int my_xpc_pipe_routine(void *xpcPipe, xpc_object_t *inDict,
                               xpc_object_t *outDict)
{
    //asm volatile ("hlt #0");
    char buf[1024];

    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    if (g_color) add_string(&ostr, RED);
    add_string(&ostr, "XPCPR %s ==> %s\n",
               getTimeStamp(buf, sizeof(buf)), xpc_copy_description(xpcPipe));
    xpc_describe(inDict, 0, &ostr);
    if (g_color) add_string(&ostr, NORMAL);
    if (g_backtrace) do_backtrace(&ostr);
    int returned = xpc_pipe_routine(xpcPipe, inDict, outDict);
    if (*outDict) {
        if (g_color) add_string(&ostr, CYAN);
        add_string(&ostr, "%s <== Reply: ", getTimeStamp(buf, sizeof(buf)));
        xpc_describe(*outDict, 0, &ostr);
        if (g_color) add_string(&ostr, NORMAL);
    }
    if (!g_skip_xpc) {
        print_string(&ostr);
    }
    return returned;
}

static xpc_connection_t my_xpc_connection_create(const char *name,
                                                 dispatch_queue_t targetq)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    add_string(&ostr, "xpc_connection_create(\"%s\", targetq=%p);\n",
               name, targetq);
    xpc_connection_t returned = xpc_connection_create(name, targetq);
    add_string(&ostr, "Returning %p\n", returned);
    if (!g_skip_xpc) {
        print_string(&ostr);
    }
    return returned;
}

extern void *_libxpc_initializer(void);
static void *my_libxpc_initializer(void)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    add_string(&ostr, "In XPC Initializer..\n");
    if (!g_skip_xpc) {
        print_string(&ostr);
    }
    return (_libxpc_initializer());
}

static xpc_type_t my_xpc_get_type(xpc_object_t obj)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    xpc_type_t returned = xpc_get_type(obj);

    add_string(&ostr, "Got type: %s\n", xpc_copy_description(obj));
    if (!g_skip_xpc) {
        print_string(&ostr);
    }
    return (returned);

}

static const char *my_xpc_dictionary_get_string(xpc_object_t dictionary,
                                                const char *key)
{
    //asm volatile ("hlt #0");
    char buf[1024];

    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    if (!g_noIncoming)
    {
        if (processedDict == dictionary) {
            add_string(&ostr, "Already processed message for key %s\n", key);
        } else {
            // Get mutex so we don't botch prints if already in print
            processedDict = dictionary;
            void *conn = xpc_dictionary_get_connection(dictionary);
            id++;
            if (conn) {
                if (g_color) add_string(&ostr, CYAN);
                add_string(&ostr, "%s (%d) Incoming <==  ",
                           getTimeStamp(buf, sizeof(buf)), id);
                xpc_describe(conn, 0, &ostr);
                if (g_color) add_string(&ostr, NORMAL);
            }

            xpc_describe(dictionary, 0, &ostr);
        }
    }
    if (!g_skip_xpc) {
        print_string(&ostr);
    }
    return xpc_dictionary_get_string(dictionary, key);
}

int64_t my_xpc_dictionary_get_int64(xpc_object_t dictionary, const char *key)
{
    //asm volatile ("hlt #0");
    char buf[1024];

    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    if (!g_noIncoming)
    {
        if (processedDict == dictionary) {
            add_string(&ostr, "Already processed message for key %s\n", key);
        }
        else {
            processedDict = dictionary;
            void *conn = xpc_dictionary_get_connection(dictionary);
            id++;
            if (conn) {
                if (g_color) add_string(&ostr, CYAN);
                add_string(&ostr, "%s (%d) Incoming <==  ",
                           getTimeStamp(buf, sizeof(buf)), id);
                xpc_describe(conn, 0, &ostr);
                if (g_color) add_string(&ostr, NORMAL);
            }
            xpc_describe(dictionary, 0, &ostr);
        }
    }
    if (!g_skip_xpc) {
        print_string(&ostr);
    }
    return xpc_dictionary_get_int64(dictionary, key);
}

static int my_sleep(unsigned int seconds)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    add_string(&ostr, "my_sleep: before real sleep()\n");
    int rc = sleep(seconds);
    add_string(&ostr, "my_sleep: after real sleep()\n");
    print_string(&ostr);
    return rc;
}

static kern_return_t my_IOServiceAddMatchingNotification(
                                            IONotificationPortRef notifyPort,
                                            const io_name_t notificationType,
                                            CFDictionaryRef matching,
                                            IOServiceMatchingCallback callback,
                                            void *refCon,
                                            io_iterator_t *notification)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr,
               "my_IOServiceAddMatchingNotification: %p %p %p %p %p %p\n",
               notifyPort, notificationType, matching, callback, refCon,
               notification);
    kern_return_t ret = IOServiceAddMatchingNotification(notifyPort,
                                                         notificationType,
                                                         matching, callback,
                                                         refCon, notification);
    add_string(&ostr, "my_IOServiceAddMatchingNotification: returned %p\n",
               ret);
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    return ret;
}

static kern_return_t my_IOServiceAddNotification(mach_port_t masterPort,
                                              const io_name_t notificationType,
                                              CFDictionaryRef matching,
                                              mach_port_t wakePort,
                                              uintptr_t reference,
                                              io_iterator_t *notification)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr, "my_IOServiceAddNotification: %p %p %p %p %p %p\n",
               masterPort, notificationType, matching, wakePort, reference,
               notification);
    kern_return_t ret = IOServiceAddNotification(masterPort, notificationType,
                                                 matching, wakePort, reference,
                                                 notification);
    add_string(&ostr, "my_IOServiceAddNotification: returned %p\n", ret);
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    return ret;
}

static io_object_t my_IOIteratorNext(io_iterator_t iterator)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr, "my_IOIteratorNext: %p\n", iterator);
    kern_return_t ret = IOIteratorNext(iterator);
    add_string(&ostr, "my_IOIteratorNext: returned %p\n", ret);
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    return ret;
}

static kern_return_t my_IOServiceGetMatchingServices(mach_port_t masterPort,
                                                     CFDictionaryRef matching,
                                                     io_iterator_t *existing)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr, "my_IOServiceGetMatchingServices: %p %p %p\n",
               masterPort, matching, existing);
    kern_return_t ret = IOServiceGetMatchingServices(masterPort, matching,
                                                     existing);
    add_string(&ostr, "my_IOServiceGetMatchingServices: returned %p\n", ret);
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    return ret;
}

static io_service_t my_IOServiceGetMatchingService(mach_port_t masterPort,
                                                   CFDictionaryRef matching)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr, "my_IOServiceGetMatchingService: %p %p\n", masterPort,
               matching);
    io_service_t ret = IOServiceGetMatchingService(masterPort, matching);
    add_string(&ostr, "my_IOServiceGetMatchingService: returned %p\n", ret);
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    return ret;
}

static CFMutableDictionaryRef my_IOServiceMatching(const char *name)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr, "my_IOServiceMatching: %s\n", name);
    CFMutableDictionaryRef ret = IOServiceMatching(name);
    add_string(&ostr, "my_IOServiceMatching: returned %p\n", ret);
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    return ret;
}

static kern_return_t my_IOServiceOpen(io_service_t service,
                                      task_port_t owningTask,
                                      uint32_t type, io_connect_t *connect)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    if (g_color) add_string(&ostr, YELLOW);
    kern_return_t ret = KERN_SUCCESS;
    add_string(&ostr, "my_IOServiceOpen: %p\n", service);
    //TODO: JONATHANA define 1
    if (g_fake_resp && (APPLE_KEY_STORE_SRV_MATCH == service)) {
        *connect = APPLE_KEY_STORE_CONNECTION;
    } else {
        ret = IOServiceOpen(service, owningTask, type, connect);
    }
    add_string(&ostr, "my_IOServiceOpen: returned: %p\n", ret);
    add_string(&ostr, "my_IOServiceOpen: connect: %p\n", connect);
    if (NULL != connect) {
        add_string(&ostr, "my_IOServiceOpen: *connect: %p\n", *connect);
    }
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    return ret;
}

static io_registry_entry_t my_IORegistryEntryFromPath(mach_port_t masterPort,
                                                      const io_string_t path)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr, "my_IORegistryEntryFromPath: %s (%p)\n", path,
               masterPort);
    io_registry_entry_t ret = 0;
    if (g_fake_resp &&
        (0 == strcmp(path, "IOService:/IOResources/AppleKeyStore"))) {
        ret = APPLE_KEY_STORE_SRV_MATCH;
    } else if (g_fake_resp &&
        //TODO: JONATHANA maybe just remove from the device tree to begin with?
        (0 == strcmp(path, "IODeviceTree:/arm-io/pearl-sep"))) {
        ret = 0;
    } else {
        ret = IORegistryEntryFromPath(masterPort, path);
    }
    add_string(&ostr, "my_IORegistryEntryFromPath: returned: %p\n", ret);
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    return ret;
}

static CFTypeRef fake_mganswer(CFStringRef s, bool *found_answer,
                               uint32_t *type)
{
    *found_answer = true;

    if (!g_fake_resp) {
        *found_answer = false;
        return NULL;
    }
    if (kCFCompareEqualTo == CFStringCompare(s, CFSTR("HasSEP"), 0)) {
        if (type) *type = 0xb;
    }
    if (kCFCompareEqualTo ==
                                //SecureElement
        CFStringCompare(s, CFSTR("0dnM19zBqLw5ZPhIo4GEkg"), 0)) {
        if (type) *type = 0xb;
        return kCFBooleanTrue;
    }
    if (kCFCompareEqualTo ==
                                //ArtworkTraits
                                //answer should be some kind of dict
                                //NULL should work well enough for now
        CFStringCompare(s, CFSTR("oPeik/9e8lQWMszEjbPzng"), 0)) {
        if (type) *type = 0;
        return NULL;
    }
    if (kCFCompareEqualTo ==
                                //InDiagnosticsMode
        CFStringCompare(s, CFSTR("3kmXfug8VcxLI5yEmsqQKw"), 0)) {
        if (type) *type = 0xb;
        return kCFBooleanFalse;
    }
    if (kCFCompareEqualTo ==
        CFStringCompare(s, CFSTR("InternalBuild"), 0)) {
        if (type) *type = 0xb;
        return kCFBooleanFalse;
    }
    if (kCFCompareEqualTo ==
        CFStringCompare(s, CFSTR("touch-id"), 0)) {
        if (type) *type = 0xb;
        return kCFBooleanFalse;
    }
    if (kCFCompareEqualTo ==
        CFStringCompare(s, CFSTR("displayport"), 0)) {
        if (type) *type = 0xb;
        return kCFBooleanFalse;
    }
    if (kCFCompareEqualTo ==
        CFStringCompare(s, CFSTR("OLEDDisplay"), 0)) {
        if (type) *type = 0xb;
        return kCFBooleanFalse;
    }
    if (kCFCompareEqualTo ==
                                //PearlIDCapability (face id)
        CFStringCompare(s, CFSTR("8olRm6C1xqr7AJGpLRnpSw"), 0)) {
        if (type) *type = 0xb;
        return kCFBooleanFalse;
    }
    if (kCFCompareEqualTo ==
                                //HasMesa (touch id)
        CFStringCompare(s, CFSTR("HV7WDiidgMf7lwAu++Lk5w"), 0)) {
        if (type) *type = 0xb;
        return kCFBooleanFalse;
    }
    if (kCFCompareEqualTo ==
                                //HasExtendedColorDisplay
        CFStringCompare(s, CFSTR("Aixt/MEN2O2B7f+8m4TxUA"), 0)) {
        if (type) *type = 0xb;
        return kCFBooleanFalse;
    }
    if (kCFCompareEqualTo ==
                                //SupportsPerseus
        CFStringCompare(s, CFSTR("GdXjx1ixZYvN9Gg8iSf68A"), 0)) {
        if (type) *type = 0xb;
        return kCFBooleanFalse;
    }
    if (kCFCompareEqualTo ==
                                //DeviceSupportsASTC
        CFStringCompare(s, CFSTR("ji56BO1mUeT7Qg9RO7Er9w"), 0)) {
        if (type) *type = 0xb;
        return kCFBooleanFalse;
    }
    if (kCFCompareEqualTo ==
        CFStringCompare(s, CFSTR("main-screen-class"), 0)) {
        if (type) *type = 5;
        int64_t num = 8;
        return CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &num);
    }
    if (kCFCompareEqualTo ==
        CFStringCompare(s, CFSTR("main-screen-orientation"), 0)) {
        if (type) *type = 7;
        int64_t num = 0;
        return CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat32Type, &num);
    }
    if (kCFCompareEqualTo ==
        CFStringCompare(s, CFSTR("DeviceColorMapPolicy"), 0)) {
        if (type) *type = 5;
        int64_t num = 0;
        return CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &num);
    }
    if (kCFCompareEqualTo ==
        CFStringCompare(s, CFSTR("DeviceClassNumber"), 0)) {
        if (type) *type = 5;
        int64_t num = 1;
        return CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &num);
    }
    if (kCFCompareEqualTo ==
        CFStringCompare(s, CFSTR("main-screen-scale"), 0)) {
        if (type) *type = 7;
        int64_t num = 0x0000000040000000;
        return CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat32Type, &num);
    }
    if (kCFCompareEqualTo ==
        CFStringCompare(s, CFSTR("main-screen-width"), 0)) {
        if (type) *type = 5;
        int64_t num = 0x00000000000002ee;
        return CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &num);
    }
    if (kCFCompareEqualTo ==
        CFStringCompare(s, CFSTR("main-screen-height"), 0)) {
        if (type) *type = 5;
        int64_t num = 0x0000000000000536;
        return CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &num);
    }
    if (kCFCompareEqualTo ==
        CFStringCompare(s, CFSTR("DisplayBootRotation"), 0)) {
        if (type) *type = 5;
        int64_t num = 0;
        return CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt64Type, &num);
    }

    *found_answer = false;
    return NULL;
}

typedef uint64_t (*mgcopyanswer_func_t)(CFStringRef s, uint32_t *type);

static uint64_t my_MGCopyAnswer_internal(CFStringRef cfstr, uint32_t *type)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    uint64_t func_addr = (uint64_t)
              gum_module_find_export_by_name("/usr/lib/libMobileGestalt.dylib",
              "MGCopyAnswer");
    if (0 == func_addr) {
        add_string(&ostr,
      "my_MGCopyAnswer_internal(): gum_module_find_export_by_name() failed\n");
        print_string(&ostr);
        abort();
    }

    //my_MGCopyAnswer_internal is 8 bytes after MGCopyAnswer
    func_addr += 8;

    mgcopyanswer_func_t orig_func = (mgcopyanswer_func_t)func_addr;

    if (g_color) add_string(&ostr, GREEN);
    add_string(&ostr, "my_MGCopyAnswer_internal: ");
    cf_dumper(cfstr, 0, &ostr);
    add_string(&ostr, "\n");
    //print before call in case it is stuck
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);

    //reset output string
    str[0] = '\0';
    ostr.str = &str[0];
    bytes_left = sizeof(str);

    if (g_color) add_string(&ostr, GREEN);

    bool fake_answer = false;
    CFTypeRef altret = fake_mganswer(cfstr, &fake_answer, type);
    if (fake_answer) {
        add_string(&ostr, "my_MGCopyAnswer_internal: returning fake: ");
        if (NULL == altret) {
            add_string(&ostr, "NULL");
        } else {
            cf_dumper(altret, 0, &ostr);
        }
        add_string(&ostr, "\n");
        if (g_backtrace) do_backtrace(&ostr);
        if (g_color) add_string(&ostr, NORMAL);
        print_string(&ostr);
        return (uint64_t)altret;
    }

    CFTypeRef ret = (CFTypeRef)orig_func(cfstr, type);
    add_string(&ostr, "my_MGCopyAnswer_internal: returned: ");
    if (NULL == ret) {
        add_string(&ostr, "NULL");
    } else {
        cf_dumper(ret, 0, &ostr);
    }
    add_string(&ostr, "\n");
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    return (uint64_t)ret;
}

static void print_io_connect_ints(ostr_t *ostr,
                                  const uint64_t *input,
                                  uint32_t inputCnt,
                                  uint64_t *output,
                                  uint32_t *outputCnt)
{
    add_string(ostr, "input ints cnt: %p\n", inputCnt);
    for (uint32_t i = 0; i < inputCnt; i++) {
        add_string(ostr, "input int[%p]: %p\n", i, input[i]);
    }
    add_string(ostr, "output ints cnt: %p\n", outputCnt);
    if (NULL != outputCnt) {
        add_string(ostr, "output ints *cnt: %p\n", *outputCnt);
        for (uint32_t i = 0; i < *outputCnt; i++) {
            add_string(ostr, "output int[%p]: %p\n", i, output[i]);
        }
    }
}

static void print_io_connect_structs(ostr_t *ostr,
                                     const void *inputStruct,
                                     size_t inputStructCnt,
                                     void *outputStruct,
                                     size_t *outputStructCnt)
{
    add_string(ostr, "input struct cnt: %p\ndata: ", inputStructCnt);
    print_data(ostr, (uint8_t *)inputStruct, inputStructCnt);
    add_string(ostr, "\n");
    add_string(ostr, "output struct cnt: %p\n", outputStructCnt);
    if (NULL != outputStructCnt) {
        add_string(ostr, "output struct *cnt: %p\ndata: ", *outputStructCnt);
        print_data(ostr, (uint8_t *)outputStruct, *outputStructCnt);
        add_string(ostr, "\n");
    }
}

static kern_return_t my_IOConnectCallAsyncMethod(mach_port_t connection,
                                                 uint32_t selector,
                                                 mach_port_t wake_port,
                                                 uint64_t *reference,
                                                 uint32_t referenceCnt,
                                                 const uint64_t *input,
                                                 uint32_t inputCnt,
                                                 const void *inputStruct,
                                                 size_t inputStructCnt,
                                                 uint64_t *output,
                                                 uint32_t *outputCnt,
                                                 void *outputStruct,
                                                 size_t *outputStructCnt)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr,
      "my_IOConnectCallAsyncMethod: con: %p sel: %p ref: %p refcnt: %p\n",
               connection, selector, reference, referenceCnt);
    print_io_connect_ints(&ostr, input, inputCnt, output, outputCnt);
    print_io_connect_structs(&ostr, inputStruct, inputStructCnt,
                             outputStruct, outputStructCnt);
    //print before call in case it is stuck
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    kern_return_t ret = IOConnectCallAsyncMethod(connection, selector,
                                                 wake_port, reference,
                                                 referenceCnt, input,
                                                 inputCnt, inputStruct,
                                                 inputStructCnt, output,
                                                 outputCnt, outputStruct,
                                                 outputStructCnt);
    //reset output string
    str[0] = '\0';
    ostr.str = &str[0];
    bytes_left = sizeof(str);

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr, "my_IOConnectCallAsyncMethod: returned: %p\n", ret);
    print_io_connect_ints(&ostr, input, inputCnt, output, outputCnt);
    print_io_connect_structs(&ostr, inputStruct, inputStructCnt,
                             outputStruct, outputStructCnt);
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    return ret;
}

static kern_return_t my_IOConnectCallAsyncStructMethod(
                                                 mach_port_t connection,
                                                 uint32_t selector,
                                                 mach_port_t wake_port,
                                                 uint64_t *reference,
                                                 uint32_t referenceCnt,
                                                 const void *inputStruct,
                                                 size_t inputStructCnt,
                                                 void *outputStruct,
                                                 size_t *outputStructCnt)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr,
     "my_IOConnectCallAsyncStructMethod: con: %p sel: %p ref: %p refcnt: %p\n",
               connection, selector, reference, referenceCnt);
    print_io_connect_structs(&ostr, inputStruct, inputStructCnt,
                             outputStruct, outputStructCnt);
    //print before call in case it is stuck
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    kern_return_t ret = IOConnectCallAsyncStructMethod(connection, selector,
                                                       wake_port, reference,
                                                       referenceCnt,
                                                       inputStruct,
                                                       inputStructCnt,
                                                       outputStruct,
                                                       outputStructCnt);
    //reset output string
    str[0] = '\0';
    ostr.str = &str[0];
    bytes_left = sizeof(str);

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr, "my_IOConnectCallAsyncStructMethod: returned: %p\n",
               ret);
    print_io_connect_structs(&ostr, inputStruct, inputStructCnt,
                             outputStruct, outputStructCnt);
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    return ret;
}

static kern_return_t my_IOConnectCallAsyncScalarMethod(
                                                 mach_port_t connection,
                                                 uint32_t selector,
                                                 mach_port_t wake_port,
                                                 uint64_t *reference,
                                                 uint32_t referenceCnt,
                                                 const uint64_t *input,
                                                 uint32_t inputCnt,
                                                 uint64_t *output,
                                                 uint32_t *outputCnt)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr,
     "my_IOConnectCallAsyncScalarMethod: con: %p sel: %p ref: %p refcnt: %p\n",
               connection, selector, reference, referenceCnt);
    print_io_connect_ints(&ostr, input, inputCnt, output, outputCnt);
    //print before call in case it is stuck
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    kern_return_t ret = IOConnectCallAsyncScalarMethod(connection, selector,
                                                       wake_port, reference,
                                                       referenceCnt, input,
                                                       inputCnt,
                                                       output,
                                                       outputCnt);
    //reset output string
    str[0] = '\0';
    ostr.str = &str[0];
    bytes_left = sizeof(str);

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr, "my_IOConnectCallAsyncScalarMethod: returned: %p\n",
               ret);
    print_io_connect_ints(&ostr, input, inputCnt, output, outputCnt);
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    return ret;
}

static kern_return_t my_IOConnectCallMethod(mach_port_t connection,
                                            uint32_t selector,
                                            const uint64_t *input,
                                            uint32_t inputCnt,
                                            const void *inputStruct,
                                            size_t inputStructCnt,
                                            uint64_t *output,
                                            uint32_t *outputCnt,
                                            void *outputStruct,
                                            size_t *outputStructCnt)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr, "my_IOConnectCallMethod: con: %p sel: %p\n",
               connection, selector);
    print_io_connect_ints(&ostr, input, inputCnt, output, outputCnt);
    print_io_connect_structs(&ostr, inputStruct, inputStructCnt,
                             outputStruct, outputStructCnt);
    //print before call in case it is stuck
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    kern_return_t ret = KERN_SUCCESS;
    if (g_fake_resp && (APPLE_KEY_STORE_CONNECTION == connection)
        && (0 == selector)) {
        ret = KERN_SUCCESS;
        //kernel method name: AppleKeyStore::se_rm_support_is_set()
    } else if (g_fake_resp && (APPLE_KEY_STORE_CONNECTION == connection)
               && (0x6b == selector)) {
        output[0] = 0;
        ret = KERN_SUCCESS;
        //kernel method name: AppleKeyStore::se_indicate_rm_support()
    } else if (g_fake_resp && (APPLE_KEY_STORE_CONNECTION == connection)
               && (0x5e == selector)) {
        ret = KERN_SUCCESS;
        //kernel method name: AppleKeyStoreUserClient::effective_bag_handle()
    } else if (g_fake_resp && (APPLE_KEY_STORE_CONNECTION == connection)
               && (7 == selector)) {
        output[0] = 0x410006;
        ret = KERN_SUCCESS;
    } else if (g_fake_resp && (APPLE_KEY_STORE_CONNECTION == connection)
               && (0x11 == selector)) {
        memcpy(outputStruct,
               "1[0\x08\x0c\x03sas\x02\x01\x000\x07\x0c\x02sb\x02\x01\x000\x08\x0c\x03sfa\x02\x01\x000\x09\x0c\x04sgpe\x02\x01\x000\x08\x0c\x03sgs\x02\x01\x000\x08\x0c\x03sls\x02\x01\x030\x07\x0c\x02sr\x02\x01\x000\x09\x0c\x04srcd\x02\x01\x000\x09\x0c\x02ss\x02\x03A\x00\x06",
               0x5d);
        *outputStructCnt = 0x5d;
        ret = KERN_SUCCESS;
    } else {
        ret = IOConnectCallMethod(connection, selector, input, inputCnt,
                                  inputStruct, inputStructCnt, output,
                                  outputCnt, outputStruct, outputStructCnt);
    }
    //reset output string
    str[0] = '\0';
    ostr.str = &str[0];
    bytes_left = sizeof(str);

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr, "my_IOConnectCallMethod: returned: %p\n", ret);
    print_io_connect_ints(&ostr, input, inputCnt, output, outputCnt);
    print_io_connect_structs(&ostr, inputStruct, inputStructCnt,
                             outputStruct, outputStructCnt);
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    return ret;
}

static kern_return_t my_IOConnectCallScalarMethod(mach_port_t connection,
                                                  uint32_t selector,
                                                  const uint64_t *input,
                                                  uint32_t inputCnt,
                                                  uint64_t *output,
                                                  uint32_t *outputCnt)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr, "my_IOConnectCallScalarMethod: con: %p sel: %p\n",
               connection, selector);
    print_io_connect_ints(&ostr, input, inputCnt, output, outputCnt);
    //print before call in case it is stuck
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    kern_return_t ret = IOConnectCallScalarMethod(connection, selector, input,
                                                  inputCnt, output, outputCnt);
    //reset output string
    str[0] = '\0';
    ostr.str = &str[0];
    bytes_left = sizeof(str);

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr, "my_IOConnectCallScalarMethod: returned: %p\n", ret);
    print_io_connect_ints(&ostr, input, inputCnt, output, outputCnt);
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    return ret;
}

kern_return_t my_IOConnectCallStructMethod(mach_port_t connection,
                                           uint32_t selector,
                                           const void *inputStruct,
                                           size_t inputStructCnt,
                                           void *outputStruct,
                                           size_t *outputStructCnt)
{
    //asm volatile ("hlt #0");
    char str[PRINT_STRING_LEN] = "";
    size_t bytes_left = sizeof(str);
    ostr_t ostr = {
        .orig_str = &str[0],
        .str = &str[0],
        .bytes_left = &bytes_left,
    };

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr, "my_IOConnectCallStructMethod: con: %p sel: %p\n",
               connection, selector);
    print_io_connect_structs(&ostr, inputStruct, inputStructCnt,
                             outputStruct, outputStructCnt);
    //print before call in case it is stuck
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    kern_return_t ret = IOConnectCallStructMethod(connection, selector,
                                            inputStruct, inputStructCnt, 
                                            outputStruct, outputStructCnt);
    //reset output string
    str[0] = '\0';
    ostr.str = &str[0];
    bytes_left = sizeof(str);

    if (g_color) add_string(&ostr, YELLOW);
    add_string(&ostr, "my_IOConnectCallStructMethod: returned: %p\n", ret);
    print_io_connect_structs(&ostr, inputStruct, inputStructCnt,
                             outputStruct, outputStructCnt);
    if (g_backtrace) do_backtrace(&ostr);
    if (g_color) add_string(&ostr, NORMAL);
    print_string(&ostr);
    return ret;
}

__attribute ((constructor)) void _init(void)
{
    //asm volatile ("hlt #0");

    char dirname[] = "/tmp";
    char *env_dirname = NULL;

    if (getenv("XPOCE_BACKTRACE")) {
        g_backtrace++;
    }
    if (getenv("XPOCE_HEX")) {
        g_hex++;
    }
    if (getenv("XPOCE_NODESC")) {
        g_myDesc = 0;
    }
    if (getenv("XPOCE_COLOR")) {
        g_color = 1;
    }
    if (access("/tmp/xpoce_color", R_OK) == 0) {
        g_color = 1;
    }
    if (access("/tmp/xpoce_backtrace", R_OK) == 0) {
        g_backtrace = 1;
    }
    if (access("/tmp/xpose_nodesc", R_OK) == 0) {
        g_myDesc = 0;
    }
    if (getenv("XPOCE_NOINC")) {
        g_noIncoming = 1;
    }
    if (getenv("XPOCE_FAKE_RESP")) {
        g_fake_resp = 1;
    }
    if (getenv("XPOCE_SKIP_XPC")) {
        g_skip_xpc = 1;
    }
    if (getenv("XPOCE_FILE_OUT")) {
        g_file_out = 1;
    }
    if (g_file_out) {
        env_dirname = getenv("XPOCE_DIRNAME");
        if (NULL == env_dirname) {
            env_dirname = dirname;
        }
        if (getenv("XPOCE_OUT")) {
            outputf = stderr;
        } else {
            char filename[1024];
            snprintf(filename, sizeof(filename), "%s/%s.%d.XPoCe",
                     env_dirname, getprogname() ,getpid());
            outputf = fopen(filename, "w");
            //wait for the r/w mount to be mounted
            while(NULL == outputf) {
                outputf = fopen(filename, "w");
                sleep(1);
            }
        }
        if (!outputf)
        {
            //we have a problem if we can't open.. just opt for stderr in
            //this case but stderr might be redirected to /dev/null..
            outputf = stderr;
        }
    }

    gum_init_embedded();

    GumInterceptor *interceptor = gum_interceptor_obtain();

    /*Transactions are optional but improve performance with multiple hooks.*/
    gum_interceptor_begin_transaction(interceptor);

    uint64_t i = 0;
    while (NULL != hooks[i].dylib_path) {
        gpointer func_addr =
            (gpointer)gum_module_find_export_by_name(hooks[i].dylib_path,
                                                     hooks[i].func_name);
        //fprintf(stderr, "hook: func_name: %s lib: %s func_addr: 0x%016llx\n",
        //        hooks[i].func_name, hooks[i].dylib_path, (uint64_t)func_addr);
        if (NULL == func_addr) {
            fprintf(stderr, "hook: gum_module_find_export_by_name() failed\n");
            abort();
        }
        int ret = gum_interceptor_replace(interceptor,
                                          func_addr + hooks[i].func_offset,
                                          hooks[i].new_func, NULL);
        if (GUM_REPLACE_OK != ret) {
            fprintf(stderr,
                    "hook: gum_interceptor_replace() failed with %d\n", ret);
            abort();
        }
        i++;
    }

    gum_interceptor_end_transaction(interceptor);
} // _init

