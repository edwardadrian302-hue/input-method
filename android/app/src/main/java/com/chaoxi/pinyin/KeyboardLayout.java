package com.chaoxi.pinyin;

import java.util.ArrayList;
import java.util.List;

/** Geometry in dp, shared by native key placement and the developer layout preview. */
public final class KeyboardLayout {
    public static final int LETTERS=0, NUMBERS=1, SYMBOLS=2;
    public static final int NORMAL=0, FUNCTION=1, FLAT=2, CANDIDATE=3, PUNCTUATION=4;
    public static final class Key {
        public final String action, label, hint;
        public final float x,y,width,height;
        public final int style;
        public Key(String action,String label,String hint,float x,float y,float width,float height,int style){
            this.action=action;this.label=label;this.hint=hint;this.x=x;this.y=y;this.width=width;this.height=height;this.style=style;
        }
    }
    public static final class Layout {
        public final List<Key> keys=new ArrayList<>();
        public final float height;
        Layout(float height){this.height=height;}
        void key(String action,String label,String hint,float x,float y,float width,float height,int style){keys.add(new Key(action,label,hint,x,y,width,height,style));}
    }
    private static final float PAD=4, GAP=5;
    public static float rowHeight(boolean landscape,boolean nine,int page,boolean english){
        if(landscape)return 46;
        if(page==LETTERS)return nine&&!english?54:53;
        return 60;
    }
    public static Layout create(float width,float rowHeight,boolean nine,int page,boolean english,boolean uppercase){
        float bottomHeight=rowHeight>=55||(nine&&!english&&page==LETTERS&&rowHeight>=50)?50:44;
        Layout layout=new Layout(rowHeight*3+bottomHeight+PAD*2);
        if((nine&&!english&&page==LETTERS)||page==NUMBERS){
            float rail=48,functions=50,centerX=PAD+rail+GAP;
            float centerWidth=width-PAD*2-rail-functions-GAP*2;
            float cell=(centerWidth-GAP*2)/3;
            String[] marks=rowHeight<50?new String[]{",",".","?"}:new String[]{",",".","?","!"};
            String[] captions=english?marks:rowHeight<50?new String[]{"，","。","？"}:new String[]{"，","。","？","！"};
            float sideHeight=rowHeight*3/marks.length;
            for(int i=0;i<marks.length;i++)layout.key(marks[i],captions[i],"",PAD,PAD+i*sideHeight,rail,sideHeight-GAP,PUNCTUATION);
            String[] letters={"分词","ABC","DEF","GHI","JKL","MNO","PQRS","TUV","WXYZ"};
            for(int i=0;i<9;i++){
                String digit=Integer.toString(i+1);boolean numeric=page==NUMBERS;
                layout.key(numeric?digit:digit+"\n"+letters[i],numeric?digit:letters[i],numeric?"":digit,
                    centerX+(i%3)*(cell+GAP),PAD+(i/3)*rowHeight,cell,rowHeight-GAP,NORMAL);
            }
            float right=width-PAD-functions;
            layout.key("⌫","⌫","",right,PAD,functions,rowHeight-GAP,FUNCTION);
            layout.key(page==NUMBERS?"ABC":"重输",page==NUMBERS?"返回":"重输","",right,PAD+rowHeight,functions,rowHeight-GAP,FUNCTION);
            layout.key("0","0","",right,PAD+rowHeight*2,functions,rowHeight-GAP,FUNCTION);
        }else if(page==SYMBOLS){
            String[][] symbols={{",",".","?","!",";",":","@"},{"#","%","&","*","(",")","_"},{"/","\"","'","+","-","=","⌫"}};
            for(int row=0;row<3;row++)gridRow(layout,symbols[row],width,PAD+row*rowHeight,rowHeight,0,uppercase);
        }else{
            gridRow(layout,new String[]{"q","w","e","r","t","y","u","i","o","p"},width,PAD,rowHeight,0,uppercase);
            gridRow(layout,new String[]{"a","s","d","f","g","h","j","k","l"},width,PAD+rowHeight,rowHeight,15,uppercase);
            gridRow(layout,new String[]{"⇧","z","x","c","v","b","n","m","⌫"},width,PAD+rowHeight*2,rowHeight,0,uppercase);
        }
        float bottomY=PAD+rowHeight*3;
        float first=42,second=46,mode=56,enter=55;
        float space=width-PAD*2-first-second-mode-enter-GAP*4;
        float x=PAD;
        layout.key(page==SYMBOLS?"ABC":"符",page==SYMBOLS?"返回":"符","",x,bottomY,first,bottomHeight-GAP,FUNCTION);x+=first+GAP;
        layout.key(page==NUMBERS?"ABC":"123",page==NUMBERS?"返回":"123","",x,bottomY,second,bottomHeight-GAP,FUNCTION);x+=second+GAP;
        layout.key("空格","空格","",x,bottomY,space,bottomHeight-GAP,NORMAL);x+=space+GAP;
        layout.key(english?"英":"中",english?"英 / 中":"中 / 英","",x,bottomY,mode,bottomHeight-GAP,FUNCTION);x+=mode+GAP;
        layout.key("↵","↵","",x,bottomY,enter,bottomHeight-GAP,FUNCTION);
        return layout;
    }
    private static void gridRow(Layout layout,String[] actions,float width,float y,float height,float inset,boolean uppercase){
        float cell=(width-PAD*2-inset*2-GAP*(actions.length-1))/actions.length;
        for(int i=0;i<actions.length;i++){
            String action=actions[i],label=uppercase?action.toUpperCase(java.util.Locale.ROOT):action;
            int style=action.equals("⇧")||action.equals("⌫")?FUNCTION:NORMAL;
            layout.key(action,label,"",PAD+inset+i*(cell+GAP),y,cell,height-GAP,style);
        }
    }
    private KeyboardLayout(){}
}
