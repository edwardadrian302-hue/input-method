package com.chaoxi.pinyin;
final class EngineBridge {
    static { System.loadLibrary("chaoxi"); }
    static native void initialize(String root, String home);
    static native void setNineKey(boolean enabled);
    static native int[] settings(int words,int phrases,boolean learning,boolean completion,int length,boolean save);
    static native void privacy(boolean suppressLearning);
    static native String[] chooseSpelling(String spelling);
    static native String[] request(int operation, int key, int value);
    private EngineBridge() {}
}
