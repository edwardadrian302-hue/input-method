package com.chaoxi.pinyin;
final class KeyboardDefaults {
    static final boolean NINE_KEY=true;
    static final int WORDS=8192, PHRASES=4096, LENGTH=4;
    static int[] state(int learned,int personal){return new int[]{1,1,Math.max(WORDS,learned),Math.max(PHRASES,personal),learned,personal,LENGTH};}
    private KeyboardDefaults(){}
}
