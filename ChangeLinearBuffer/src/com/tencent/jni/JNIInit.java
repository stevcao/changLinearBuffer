package com.tencent.jni;

public class JNIInit {
	
	/**
	 * �ı������ڴ�������Ļ���
	 */
	public static native void changeLinearBuffer();
	
	
	static {
		System.loadLibrary("jni-init");
	}
}
