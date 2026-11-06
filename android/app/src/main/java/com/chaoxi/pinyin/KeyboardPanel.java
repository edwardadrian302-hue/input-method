package com.chaoxi.pinyin;
import android.content.Context;
import android.content.res.Configuration;
import android.view.ViewGroup;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import java.util.function.Consumer;

// Internal programmatic view; no XML inflation constructor is needed.
@android.annotation.SuppressLint("ViewConstructor")
final class KeyboardPanel extends ViewGroup {
    private boolean nine,english,uppercase;
    private int page,layoutWidth;
    private float rowHeight;
    private ScrollView spellingRail;
    private String[] spellings=new String[0];
    private String prefix="";
    private final Consumer<String> listener;
    private KeyboardLayout.Layout layout;
    private final float density;
    KeyboardPanel(Context context,Consumer<String> listener){super(context);this.listener=listener;density=getResources().getDisplayMetrics().density;setBackgroundColor(0xffd5d9df);update(false,0,false,false);}
    void update(boolean nine,int page,boolean english,boolean uppercase){
        cancelTouches();this.nine=nine;this.page=page;this.english=english;this.uppercase=uppercase;
        rowHeight=KeyboardLayout.rowHeight(getResources().getConfiguration().orientation==Configuration.ORIENTATION_LANDSCAPE,nine,page,english);
        layoutWidth=getWidth()>0?getWidth():getResources().getDisplayMetrics().widthPixels;
        layout=KeyboardLayout.create(layoutWidth/density,rowHeight,nine,page,english,uppercase);
        removeAllViews();
        for(KeyboardLayout.Key key:layout.keys)addView(new KeyboardKey(getContext(),key.action,key.label,key.hint,key.style,()->listener.accept(key.action)),new LayoutParams(LayoutParams.WRAP_CONTENT,LayoutParams.WRAP_CONTENT));
        buildSpellingRail();requestLayout();
    }
    void setSpellings(String[] values,String selected){
        if(java.util.Arrays.equals(spellings,values)&&prefix.equals(selected))return;
        spellings=values.clone();prefix=selected;buildSpellingRail();requestLayout();
    }
    private void buildSpellingRail(){
        if(spellingRail!=null){removeView(spellingRail);spellingRail=null;}
        boolean show=nine&&!english&&page==KeyboardLayout.LETTERS&&(spellings.length>0||!prefix.isEmpty());
        for(int i=0;i<layout.keys.size();i++)if(layout.keys.get(i).style==KeyboardLayout.PUNCTUATION)getChildAt(i).setVisibility(show?View.INVISIBLE:View.VISIBLE);
        if(!show)return;
        spellingRail=new ScrollView(getContext());spellingRail.setVerticalScrollBarEnabled(true);spellingRail.setBackgroundColor(0xffeef1f5);
        LinearLayout column=new LinearLayout(getContext());column.setOrientation(LinearLayout.VERTICAL);spellingRail.addView(column);
        if(!prefix.isEmpty()){
            addSpelling(column,"-","撤销");
            String[] selected=prefix.split(" ");KeyboardKey locked=new KeyboardKey(getContext(),"locked",selected[selected.length-1],"",KeyboardLayout.FLAT,()->{});locked.accent(true);locked.setEnabled(false);column.addView(locked,new LinearLayout.LayoutParams(-1,Math.round(36*density)));
        }
        for(String value:spellings)addSpelling(column,value,value);
        addView(spellingRail,new LayoutParams(LayoutParams.WRAP_CONTENT,LayoutParams.WRAP_CONTENT));
    }
    private void addSpelling(LinearLayout column,String value,String label){
        KeyboardKey key=new KeyboardKey(getContext(),"spelling",label,"",KeyboardLayout.FLAT,()->listener.accept("pinyin:"+value));
        column.addView(key,new LinearLayout.LayoutParams(-1,Math.round(40*density)));
    }
    @Override protected void onConfigurationChanged(Configuration configuration){super.onConfigurationChanged(configuration);update(nine,page,english,uppercase);}
    void cancelTouches(){for(int i=0;i<getChildCount();i++)if(getChildAt(i) instanceof KeyboardKey)((KeyboardKey)getChildAt(i)).cancelTouch();if(spellingRail!=null)cancelRail(spellingRail);}
    private void cancelRail(android.view.ViewGroup group){for(int i=0;i<group.getChildCount();i++){View child=group.getChildAt(i);if(child instanceof KeyboardKey)((KeyboardKey)child).cancelTouch();else if(child instanceof android.view.ViewGroup)cancelRail((android.view.ViewGroup)child);}}
    @Override protected void onMeasure(int widthSpec,int heightSpec){
        int width=MeasureSpec.getSize(widthSpec);
        if(width!=layoutWidth){layoutWidth=width;layout=KeyboardLayout.create(width/density,rowHeight,nine,page,english,uppercase);}
        for(int i=0;i<layout.keys.size();i++)getChildAt(i).measure(MeasureSpec.makeMeasureSpec(Math.round(layout.keys.get(i).width*density),MeasureSpec.EXACTLY),MeasureSpec.makeMeasureSpec(Math.round(layout.keys.get(i).height*density),MeasureSpec.EXACTLY));
        if(spellingRail!=null)spellingRail.measure(MeasureSpec.makeMeasureSpec(Math.round(48*density),MeasureSpec.EXACTLY),MeasureSpec.makeMeasureSpec(Math.round((rowHeight*3-5)*density),MeasureSpec.EXACTLY));
        setMeasuredDimension(width,Math.round(layout.height*density));
    }
    @Override protected void onLayout(boolean changed,int l,int t,int r,int b){if(layout==null)return;for(int i=0;i<layout.keys.size();i++){KeyboardLayout.Key k=layout.keys.get(i);int x=Math.round(k.x*density),y=Math.round(k.y*density);getChildAt(i).layout(x,y,x+Math.round(k.width*density),y+Math.round(k.height*density));}if(spellingRail!=null){int x=Math.round(4*density),y=Math.round(4*density);spellingRail.layout(x,y,x+Math.round(48*density),y+Math.round((rowHeight*3-5)*density));}}
}
