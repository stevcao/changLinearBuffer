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


#include "com_tencent_jni_JNIInit.h"

static JavaVM* gvm = NULL;

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
	gvm = vm;
	return JNI_VERSION_1_4;
}

/**
 * 将char*类型转化成jstring
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
 * LinearAllocHdr的定义
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
 * 修改LinearAllocHdr对象的缓冲区
 */
JNIEXPORT void JNICALL Java_com_tencent_jni_JNIInit_changeLinearBuffer
  (JNIEnv *env, jclass clazz){
	char* info;
	void * dvmFile = dlopen("/system/lib/libdvm.so",RTLD_LAZY);
	if(dvmFile) {
		info = "system found!";
		long* dvmGlobals = (long*)dlsym(dvmFile,"gDvm");
		if(dvmGlobals) {
			if(gvm != NULL) {
				info = "dvmGlobals found!";
				int i = findVMList(dvmGlobals);
				if(i != -1) {
					LOGD("vmlist found! %d from dvmGlobals", i);
					int j = findLinearAllocHdr(dvmGlobals,i,DEFAULT_MAX_LENGTH);
					if(j == -1) goto exit;
					LOGD("LinearAlocHdr struct found,%d from vmlist.",j);

					long* linearAlocPtr = getLinearAllocHdrAddr(dvmGlobals,i,j);

					int result = changeLinearBuffer(linearAlocPtr);

					if (result == -1) {
						info = "change linear buffer failed!";
					} else {
						info = "change linear buffer success!";
					}

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
	LOGD("%s",info);
	return;
}

/**
 * 打印缓存信息
 */
JNIEXPORT jstring JNICALL Java_com_tencent_jni_JNIInit_logBufferInfo
  (JNIEnv * env, jclass clazz) {
	char buffer[100];
	char* info;
	void * dvmFile = dlopen("/system/lib/libdvm.so",RTLD_LAZY);
		if(dvmFile) {
			info = "system found!";
			long* dvmGlobals = (long*)dlsym(dvmFile,"gDvm");
			if(dvmGlobals) {
				if(gvm != NULL) {
					int i = findVMList(dvmGlobals);
					if(i != -1) {
						LOGD("vmlist found! %d from dvmGlobals", i);
						int j = findLinearAllocHdr(dvmGlobals,i,NEW_MAX_LENGTH);
						if(j != -1) {
							LOGD("LinearAlocHdr struct found,%d from vmlist.",j);

							long* linearAlocPtr = getLinearAllocHdrAddr(dvmGlobals,i,j);
							int* curOffset = getMapCur(linearAlocPtr);
							char* mapAddr = getMapAddr(linearAlocPtr);
							int* mapLength = getMapLength(linearAlocPtr);
							int* firstOffset = getMapFirst(linearAlocPtr);
							pthread_mutex_t* lock = getLock(linearAlocPtr);
							pthread_mutex_lock(lock);//加锁
							sprintf(buffer,"addr:%X,cur:%d,first:%d,len:%d",mapAddr,*curOffset,*firstOffset,*mapLength);
							pthread_mutex_unlock(lock);//解锁
							info = buffer;
							LOGD("%s",info);
						} else {
							info = "buffer not found!";
						}
					} else {
						info = "vmlist not found!";
					}
				} else {
					info = "vm null";
				}
			} else {
				info = "dvmGlobals found!";
			}
			dlclose(dvmFile);
		} else {
			info = "libdvm.so not found!";
		}
		return strToJstring(env,info);
}

int changeLinearBuffer(long* linearAlocPtr) {
	char buffer[100];
	int* curOffset = getMapCur(linearAlocPtr);
	char* mapAddr = getMapAddr(linearAlocPtr);
	int* mapLength = getMapLength(linearAlocPtr);
	int* firstOffset = getMapFirst(linearAlocPtr);
	pthread_mutex_t* lock = getLock(linearAlocPtr);
	long oldmapAddr = *((long*)mapAddr);
	int oldFirst = *firstOffset;
	int oldCur = *curOffset;
	int oldMapLength = *mapLength;
	int mapAddrOffset = 1 + sizeof(pthread_mutex_t)/sizeof(long*);
	//同步
	{
		LOGD("change buffer start. ");
		pthread_mutex_lock(lock);//加锁

		//此处修改LinearAllocHdr的值
		*mapLength = NEW_MAX_LENGTH;
		*curOffset = *firstOffset = (BLOCK_ALIGN-HEADER_EXTRA) + SYSTEM_PAGE_SIZE;
		//创建一个新的缓冲区
		mapAddr = (char*)mmap(NULL, (size_t)mapLength, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
		if (mapAddr == MAP_FAILED) {
			LOGE("LinearAlloc mmap failed");
			return -1;
		}
		//将结构体中的缓存区首地址改成新的缓冲区地址
		long* mapAddrPtr = (linearAlocPtr + mapAddrOffset);
		*mapAddrPtr = (long)mapAddr;

		pthread_mutex_unlock(lock);//解锁
		LOGD("change buffer end");
		return 1;
	}
	sprintf(buffer,"old:%d,%d,%d,%X\n",
		oldFirst,oldCur,oldMapLength,oldmapAddr);
	LOGD("%s",buffer);
	sprintf(buffer,"cur:\n %d,%d,%d,%X\n",*firstOffset,*curOffset,*mapLength,mapAddr);
	LOGD("%s",buffer);
}

long* getLinearAllocHdrAddr(long* dvmGlobals,int i,int j) {
	long linearAlocAddr = (*(dvmGlobals + i + j));
	long* linearAlocPtr = (long*)linearAlocAddr;
	return linearAlocPtr;
}

char* getMapAddr(long* linearAlocPtr) {
	char* mapAddr = (char*)(linearAlocPtr + 1 + sizeof(pthread_mutex_t)/sizeof(long*));
	return mapAddr;
}

int* getMapFirst(long* linearAlocPtr) {
	int* firstOffset = (int*)(linearAlocPtr + 1 + sizeof(pthread_mutex_t)/sizeof(long*) + 1 + 1);
	return firstOffset;
}

int* getMapLength(long* linearAlocPtr) {
	int* mapLength = (int*)(linearAlocPtr + 1 + sizeof(pthread_mutex_t)/sizeof(long*) + 1);
	return mapLength;
}

pthread_mutex_t* getLock(long* linearAlocPtr) {
	pthread_mutex_t* lock = (pthread_mutex_t*)(linearAlocPtr + 1);
	return lock;
}

int* getMapCur(long* linearAlocPtr) {
	int* curOffset = (int*)linearAlocPtr;
	return curOffset;
}

/**
 * 从globals的首地址开始遍历找到vmlist对象
 * 返回vmlist的偏移值
 */
int findVMList(long* dvmGlobals) {
	long* gvmLongPtr = (long*)gvm;
	long gvmLong = (long)gvmLongPtr;//获取gvm对象的首地址值
	int i = 0;
	while(i < MAX_GLOBALS_LENGTH) {
	//将dvmGlobals结构体的成员变量依次和gvm进行对边
		if(*(dvmGlobals+i) == gvmLong)break;
		i++;
	}
	if(i != MAX_GLOBALS_LENGTH) {
		return i;
	}
	else {
		return -1;
	}
}

/**
 *在vmlist的偏移值的基础上再寻找LinearAllocHdr结构体
 */
int findLinearAllocHdr(long* dvmGlobals,int vmListIndex,int length) {
	int j = LINEAR_VMLIST_OFFSET;
	while ( j < MAX_GLOBALS_LENGTH - vmListIndex) {
		LOGD("J = %d",j);
		if(checkIsLinearAllocPtr(dvmGlobals + vmListIndex + j,length))break;
		j++;
	}
	if(j == MAX_GLOBALS_LENGTH - vmListIndex) {
		return -1;
	} else {
		return j;
	}
}



/**
 * 判断一个内存首地址是否是LinearAllocHdr结构体的首地址，且其缓冲是5M
 * 判断原理是找到mapLength的地址，判断其是否是5*1024*1024
 */
bool checkIsLinearAllocPtr(long* ptr,int length) {
	long linearAlocAddr = (*ptr);
	long* linearAlocPtr = (long*)linearAlocAddr;
	int offset = 1 + sizeof(pthread_mutex_t)/4 + 1;
	int* mapLength = (int*)(linearAlocPtr+offset);
//	LOGD("%X",mapLength);
	if((long)mapLength < MIN_SEARCH_ADDR) {
		return false;
	}
	int value = 0;
	value = *mapLength;
	return  value == length;
}
