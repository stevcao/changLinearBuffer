/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_tencent_jni_JNIInit */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <jni.h>
#include <dlfcn.h>

#include <pthread.h>
#include <android/log.h>

/*#include <cutils/ashmem.h>*/
#include <sys/mman.h> //共享内存映射
#define LOG "jni log"
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG , LOG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO , LOG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN , LOG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR , LOG, __VA_ARGS__)

/*#include "Dalvik.h"*/
/* alignment for allocations; must be power of 2, and currently >= hdr_xtra */
#define BLOCK_ALIGN         8
/* default length of memory segment (worst case is probably "dexopt") */
#define DEFAULT_MAX_LENGTH  (5*1024*1024)/* leave enough space for a length word */
#define NEW_MAX_LENGTH (8*1024*1024)
#define SYSTEM_PAGE_SIZE (4*1024)
#define HEADER_EXTRA        4/* overload the length word */
#define LENGTHFLAG_FREE    0x80000000
#define LENGTHFLAG_RW      0x40000000
#define LENGTHFLAG_MASK    (~(LENGTHFLAG_FREE|LENGTHFLAG_RW))
#define MAX_GLOBALS_LENGTH 800 //这里暂时就定义这么大
#define LINEAR_VMLIST_OFFSET 1;

#define MIN_SEARCH_ADDR 0XE000 //防止访问非法地址，checkIsLinearAllocPtr_5m函数中，地址小于该值就不检查直接返回false

#ifndef _Included_com_tencent_jni_JNIInit
#define _Included_com_tencent_jni_JNIInit
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_tencent_jni_JNIInit
 * Method:    changeLinearBuffer
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_tencent_jni_JNIInit_changeLinearBuffer
  (JNIEnv *, jclass);

/*
 * Class:     com_tencent_jni_JNIInit
 * Method:    logBufferInfo
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_tencent_jni_JNIInit_logBufferInfo
  (JNIEnv *, jclass);

bool checkIsLinearAllocPtr(long*,int);

int findVMList(long* dvmGlobals);

int findLinearAllocHdr(long* dvmGlobals,int vmListIndex,int length);

long* getLinearAllocHdrAddr(long* dvmGlobals,int i,int j);

char* getMapAddr(long* linearAlocPtr);

int* getMapFirst(long* linearAlocPtr);

int* getMapLength(long* linearAlocPtr);

pthread_mutex_t* getLock(long* linearAlocPtr);

int* getMapCur(long* linearAlocPtr);

int changeLinearBuffer(long* linearAlocPtr);

#ifdef __cplusplus
}
#endif
#endif
