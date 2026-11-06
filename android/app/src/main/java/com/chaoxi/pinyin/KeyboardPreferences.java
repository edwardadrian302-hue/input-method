package com.chaoxi.pinyin;
import android.content.Context;
final class KeyboardPreferences {
    static boolean nineKey(Context context){return context.getSharedPreferences("keyboard",Context.MODE_PRIVATE).getBoolean("nineKey",KeyboardDefaults.NINE_KEY);}
    static boolean nineKey(Context context,boolean value){return context.getSharedPreferences("keyboard",Context.MODE_PRIVATE).edit().putBoolean("nineKey",value).commit();}
    private KeyboardPreferences(){}
}
