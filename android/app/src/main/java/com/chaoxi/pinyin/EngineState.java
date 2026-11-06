package com.chaoxi.pinyin;
import java.util.Arrays;

/** JNI transport only; the desktop v5 IPC structure remains unchanged. */
final class EngineState {
    final String commit,preedit,prefix;
    final int page;
    final boolean last,prediction,eaten;
    final String[] candidates,spellings;
    EngineState(String[] values){
        if(values==null||values.length<8)throw new IllegalArgumentException("Incomplete engine state");
        int count=Integer.parseInt(values[6]);
        if(count<0||count>9||values.length<8+count)throw new IllegalArgumentException("Invalid candidate count");
        commit=values[0];preedit=values[1];page=Integer.parseInt(values[2]);last="1".equals(values[3]);prediction="1".equals(values[4]);eaten="1".equals(values[5]);prefix=values[7];
        candidates=Arrays.copyOfRange(values,8,8+count);spellings=Arrays.copyOfRange(values,8+count,values.length);
    }
}
