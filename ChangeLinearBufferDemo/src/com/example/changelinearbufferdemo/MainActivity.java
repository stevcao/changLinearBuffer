package com.example.changelinearbufferdemo;

import android.app.Activity;
import android.os.Bundle;
import android.view.View;
import android.view.View.OnClickListener;
import android.widget.TextView;

import com.tencent.jni.JNIInit;
import com.test.classes.Class;
import com.test.classes.Class1;
import com.test.classes.Class2;
import com.test.classes.Class3;
import com.test.classes.Class4;
import com.test.classes.Class5;
import com.test.classes.Class6;
import com.test.classes.Class7;

public class MainActivity extends Activity {

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);

		//调用改变线性内存分配器指向缓冲
		JNIInit.changeLinearBuffer();
		JNIInit.logBufferInfo();
		final TextView  tv = new TextView(this);
		tv.append("helloWorld!");
		
		tv.append("\n");
		tv.append(JNIInit.logBufferInfo());
		
		new Class().get();
		
		tv.append("\n");
		tv.append(JNIInit.logBufferInfo());
		
		new Class1().get();
		tv.append("\n");
		tv.append(JNIInit.logBufferInfo());
		
		new Class2().get();
		tv.append("\n");
		tv.append(JNIInit.logBufferInfo());
		
		new Class3().get();
		tv.append("\n");
		tv.append(JNIInit.logBufferInfo());
		
//  	    new Class4().get();
//		new Class5().get();
//		new Class6().get();
		//new Class7().get();
		

		setContentView(tv);
		
		tv.setOnClickListener(new OnClickListener() {
			
			@Override
			public void onClick(View v) {
				tv.append("\n");
				tv.append(JNIInit.logBufferInfo());
			}
		});

	}


}
