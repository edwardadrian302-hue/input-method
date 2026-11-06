package com.chaoxi.pinyin;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
public final class KeyboardLayoutTest {
    static int checks;
    static void check(boolean condition,String message){if(!condition)throw new AssertionError(message);checks++;}
    static String quote(String value){return "\""+value.replace("\\","\\\\").replace("\"","\\\"").replace("\n","\\n")+"\"";}
    static void json(KeyboardLayout.Layout layout,String name) throws Exception{
        StringBuilder out=new StringBuilder("{\"width\":360,\"height\":"+layout.height+",\"keys\":[");
        boolean first=true;for(KeyboardLayout.Key k:layout.keys){if(!first)out.append(',');first=false;
            out.append("{\"action\":").append(quote(k.action)).append(",\"label\":").append(quote(k.label)).append(",\"hint\":").append(quote(k.hint)).append(",\"x\":").append(k.x).append(",\"y\":").append(k.y).append(",\"width\":").append(k.width).append(",\"height\":").append(k.height).append(",\"style\":").append(k.style).append('}');}
        out.append("]}");Files.writeString(Path.of("build/"+name+".json"),out.toString(),StandardCharsets.UTF_8);
    }
    public static void main(String[] args) throws Exception{
        int layouts=0;
        for(int width:new int[]{320,360,411,480,640})for(int page=0;page<3;page++)for(boolean nine:new boolean[]{false,true})for(boolean english:new boolean[]{false,true}){
            KeyboardLayout.Layout layout=KeyboardLayout.create(width,KeyboardLayout.rowHeight(width>=600,nine,page,english),nine,page,english,false);layouts++;
            for(int i=0;i<layout.keys.size();i++){
                KeyboardLayout.Key k=layout.keys.get(i);check(k.width>=24&&k.height>=35,"tap target too small: "+k.action+" "+k.width+"x"+k.height);
                check(k.x>=0&&k.y>=0&&k.x+k.width<=width+.01f&&k.y+k.height<=layout.height,"key outside keyboard bounds");
                for(int j=i+1;j<layout.keys.size();j++){KeyboardLayout.Key n=layout.keys.get(j);float w=Math.min(k.x+k.width,n.x+n.width)-Math.max(k.x,n.x);float h=Math.min(k.y+k.height,n.y+n.height)-Math.max(k.y,n.y);check(w<=0||h<=0,"overlapping keys");}
            }
        }
        KeyboardLayout.Layout nine=KeyboardLayout.create(360,KeyboardLayout.rowHeight(false,true,0,false),true,0,false,false);
        check(nine.keys.stream().filter(k->k.action.startsWith("2\n")&&k.label.equals("ABC")&&k.hint.equals("2")).count()==1,"nine-key group action missing");
        for(String action:new String[]{"⌫","重输","0","空格","符","123","中","↵",",",".","?","!"})check(nine.keys.stream().anyMatch(k->k.action.equals(action)),"missing action: "+action);
        check(KeyboardDefaults.NINE_KEY,"fresh-install default is nine keys");int[] defaults=KeyboardDefaults.state(0,0);
        check(defaults[0]==1&&defaults[1]==1&&defaults[2]==8192&&defaults[3]==4096&&defaults[6]==4,"incorrect default settings");
        defaults=KeyboardDefaults.state(9000,5000);check(defaults[2]==9000&&defaults[3]==5000,"reset must preserve capacity for existing records");
        check(nine.height==220,"portrait nine-key height should be reduced while retaining bottom function row");
        for(String action:new String[]{",",".","?","!"}){
            KeyboardLayout.Key mark=nine.keys.stream().filter(k->k.action.equals(action)).findFirst().orElseThrow();
            check(mark.width==48&&mark.style==KeyboardLayout.PUNCTUATION,"punctuation rail must be wider with its own larger type style");
        }
        json(nine,"android-032-nine-layout");json(KeyboardLayout.create(360,53,false,0,false,false),"android-032-qwerty-layout");
        System.out.println("PASS: "+checks+" geometry/default checks across "+layouts+" layouts; preview geometry exported.");
    }
}
