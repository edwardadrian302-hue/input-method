package com.chaoxi.pinyin;
public final class EngineStateTest {
    private static void check(boolean condition,String message){if(!condition)throw new AssertionError(message);}
    public static void main(String[] args){
        EngineState state=new EngineState(new String[]{"","rong","0","0","0","1","9","","容","送","松","宋","荣","融","蓉","耸","诵","rong","song"});
        check(state.candidates.length==9&&state.candidates[2].equals("松"),"all candidate positions must survive JNI parsing");
        check(state.spellings.length==2&&state.spellings[1].equals("song"),"spellings must be separated from word candidates");
        state=new EngineState(new String[]{"送礼","","0","1","0","1","0",""});
        check(state.commit.equals("送礼")&&state.candidates.length==0&&state.last,"whole phrase commit and empty menu must survive transport");
        state=new EngineState(new String[]{"","song li","1","0","0","1","1","song","送礼","li"});
        check(state.page==1&&state.prefix.equals("song")&&state.spellings[0].equals("li"),"filtered page and phonetic lock must survive transport");
        boolean rejected=false;try{new EngineState(new String[]{"","","0","1","0","1","9",""});}catch(IllegalArgumentException expected){rejected=true;}
        check(rejected,"truncated menu must be rejected");
        System.out.println("PASS: JNI transport keeps nine words, spelling alternatives, filtered pages and whole phrase commits distinct.");
    }
}
