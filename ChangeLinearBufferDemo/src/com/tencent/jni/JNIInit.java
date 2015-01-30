package com.tencent.jni;

public class JNIInit {
	
	/**
	 * 改变线性内存分配器的缓存
	 */
	public static native void changeLinearBuffer();
	
	public static native String logBufferInfo();
	
	static {
		System.loadLibrary("jni-init");
	}
}
