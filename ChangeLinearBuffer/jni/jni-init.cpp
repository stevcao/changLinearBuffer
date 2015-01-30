/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <jni.h>
#include <dlfcn.h>
#include "com_tencent_jni_JNIInit.h"
#include <pthread.h>
#include <android/log.h>

/*#include <cutils/ashmem.h>*/
#include <sys/mman.h> //�����ڴ�ӳ��
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
#define NEW_MAX_LENGTH (6024)
#define SYSTEM_PAGE_SIZE (4*1024)
#define HEADER_EXTRA        4/* overload the length word */
#define LENGTHFLAG_FREE    0x80000000
#define LENGTHFLAG_RW      0x40000000
#define LENGTHFLAG_MASK    (~(LENGTHFLAG_FREE|LENGTHFLAG_RW))
#define MAX_GLOBALS_LENGTH 800 //������ʱ�Ͷ�����ô��
#define LINEAR_VMLIST_OFFSET 6;

static JavaVM* gvm = NULL;

/**
 * �ڼ��غ����и�gvm��ֵ�����ֵ������Globals.h�ж�λvmlist����
 */
jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	gvm = vm;
	return JNI_VERSION_1_4;
}

/**
 * ��char*����ת����jstring
 */
jstring strToJstring(JNIEnv* env, const char* pStr)
 {
     int        strLen    = strlen(pStr);
     jclass     jstrObj   = env->FindClass("java/lang/String");
     jmethodID  methodId  = env->GetMethodID(jstrObj, "<init>", "([BLjava/lang/String;)V");
     jbyteArray byteArray = (env)->NewByteArray(strLen);
     jstring    encode    = (env)->NewStringUTF("utf-8");

    (env)->SetByteArrayRegion(byteArray, 0, strLen, (jbyte*)pStr);
     return (jstring)(env)->NewObject(jstrObj, methodId, byteArray, encode);
 }

/*
 * LinearAllocHdr�Ķ���
 *
 typedef struct LinearAllocHdr {
	int     curOffset;
	pthread_mutex_t lock;
	char*   mapAddr;
	int     mapLength;
	int     firstOffset;
	short*  writeRefCount;
} LinearAllocHdr;

*/

/**
 * �޸�LinearAllocHdr����Ļ�����
 * �����ҵ�gDvm�ṹ��ĵ�ַ
 * Ȼ��ͨ���Ա�vmlist��gvm��ֵ����λ��vmlist��vmlist��gvm����ָ��JavaVM�Ķ���
 * Ȼ����ƫ���ҵ�LinearAllocHdr�ṹ��
 * ���ı�LinearAllocHdr�Ļ�����
 */
JNIEXPORT void JNICALL Java_com_tencent_jni_JNIInit_changeLinearBuffer
  (JNIEnv *env, jclass clazz){
	char* info;
	char buffer[100];
	void * dvmFile = dlopen("/system/lib/libdvm.so",RTLD_LAZY);
	if(dvmFile) {
		info = "system found!";
		long* dvmGlobals = (long*)dlsym(dvmFile,"gDvm");
		if(dvmGlobals) {
			if(gvm != NULL) {
				info = "dvmGlobals found!";
				long* gvmLongPtr = (long*)gvm;
				long gvmLong = (long)gvmLongPtr;//��ȡgvm������׵�ֵַ
				int i = 0;
				while(i < MAX_GLOBALS_LENGTH) {
					//��dvmGlobals�ṹ��ĳ�Ա�������κ�gvm���жԱ�
					if(*(dvmGlobals+i) == gvmLong)break;
					i++;
				}
				if(i != MAX_GLOBALS_LENGTH) {
					//��һ���ִ����ͷҪ�ģ����ֳ��̿��ܸ���Globals.h�Ĵ��룬ƫ������һ������6
					LOGD("dvmGlobals:%d,vmlist:%d",dvmGlobals,dvmGlobals + i);
					int j = LINEAR_VMLIST_OFFSET;
					while ( j < MAX_GLOBALS_LENGTH - i) {
						if(checkIsLinearAllocPtr_5m(dvmGlobals + i + j))break;
						j++;
					}
					if(j == MAX_GLOBALS_LENGTH - i) goto exit;
					LOGD("LinearAlocHdr struct found");
					long linearAlocAddr = (*(dvmGlobals+i+j));
					long* linearAlocPtr = (long*)linearAlocAddr;
					//��ȡLinearAllocHdr�ṹ���ֵ
					int offset = 0;
					int* curOffset = (int*)linearAlocPtr;
					offset += sizeof(int*)/sizeof(int*);
					pthread_mutex_t* lock = (pthread_mutex_t*)(linearAlocPtr + offset);
					offset += sizeof(pthread_mutex_t)/sizeof(long*);
					int mapAddrOffset = offset;
					char* mapAddr = (char*)(linearAlocPtr + offset);
					offset += sizeof(char*)/sizeof(long*);
					int* mapLength = (int*)(linearAlocPtr+offset);
					offset +=sizeof(int*)/sizeof(long*);
					int* firstOffset = (int*)(linearAlocPtr+offset);
					//��¼��ָ��;�������Ĵ��뽫��ı���Щָ���ֵ
					long oldmapAddr = *((long*)mapAddr);
					int oldFirst = *firstOffset;
					int oldCur = *curOffset;
					int oldMapLength = *mapLength;
					//ͬ����Ҫ�޸�LinearAllocHdr����֮ǰ��Ҫ��ȡ��ͬ��������ס����Ȼ���ٸ��仺����
					{
						LOGD("change buffer start.");
						pthread_mutex_lock(lock);//����

						//�˴��޸�LinearAllocHdr��ֵ
						*mapLength = NEW_MAX_LENGTH;
						*curOffset = *firstOffset = (BLOCK_ALIGN-HEADER_EXTRA) + SYSTEM_PAGE_SIZE;
						//����һ���µĻ�����
						mapAddr = (char*)mmap(NULL, (size_t)mapLength, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
						if (mapAddr == MAP_FAILED) {//����������ʧ��
							LOGE("LinearAlloc mmap failed");
							goto exit;
						}
						//���ṹ���еĻ������׵�ַ�ĳ��µĻ�������ַ
						long* mapAddrPtr = (linearAlocPtr + mapAddrOffset);
						*mapAddrPtr = (long)mapAddr;

						pthread_mutex_unlock(lock);//����
						LOGD("change buffer end");
					}
					sprintf(buffer,"old:\nfirst=%d,cur=%d,length=%d,addr=%X,\ncur:\n first=%d,cur=%d,length=%d,addr=%X\n",
						oldFirst,oldCur,oldMapLength,oldmapAddr,*firstOffset,*curOffset,*mapLength,mapAddr);
					info = buffer;
				}
			}
		} else {
			info = "globals not found";
		}
		dlclose(dvmFile);
	} else {
		info = "system not found!";
	}
	exit:
	LOGD("init result:\n%s",info);
	return;
}

/**
 * �ж�һ���ڴ��׵�ַ�Ƿ���LinearAllocHdr�ṹ����׵�ַ�����仺����5M
 * �ж�ԭ�����ҵ�mapLength�ĵ�ַ���ж����Ƿ���5*1024*1024
 */
bool checkIsLinearAllocPtr_5m(long* ptr) {
	long linearAlocAddr = (*ptr);
	long* linearAlocPtr = (long*)linearAlocAddr;
	int offset = 1 + sizeof(pthread_mutex_t)/4 + 1;
	int* mapLength = (int*)(linearAlocPtr+offset);
	return *mapLength == DEFAULT_MAX_LENGTH;
}
