package com.tencent.jni;

public class JNIInit {
	
	/**
	 * �ı������ڴ�������Ļ���
	 */
	public static native void changeLinearBuffer();
	
	public static native String logBufferInfo();
	
	static {
		System.loadLibrary("jni-init");
	}
}
