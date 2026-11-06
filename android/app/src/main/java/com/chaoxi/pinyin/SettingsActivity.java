package com.chaoxi.pinyin;

import android.app.Activity;
import android.app.AlertDialog;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.content.Intent;
import android.provider.Settings;
import android.text.InputType;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.view.Gravity;
import android.widget.*;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;

public final class SettingsActivity extends Activity {
    private static final int INK=0xff26333b,MUTED=0xff64727e,ACCENT=0xff247983;
    private final Handler main=new Handler(Looper.getMainLooper());
    private TextView feedback,usage;
    private EditText words,phrases;
    private CheckBox learning,completion;
    private Spinner layoutChoice,length;
    private Button save,clear,restore,retry;
    private ProgressBar progress;
    private boolean loaded,binding,busyState;
    private int[] savedState;
    private boolean savedNine;
    private int dp(int n){return Math.round(n*getResources().getDisplayMetrics().density);}
    private GradientDrawable background(int color,int radius){GradientDrawable d=new GradientDrawable();d.setColor(color);d.setCornerRadius(dp(radius));return d;}
    private TextView label(LinearLayout body,String text,int size,int color){TextView v=new TextView(this);v.setText(text);v.setTextSize(size);v.setTextColor(color);v.setPadding(0,dp(7),0,dp(7));body.addView(v);return v;}
    private LinearLayout card(LinearLayout body,String title,String note){
        LinearLayout card=new LinearLayout(this);card.setOrientation(LinearLayout.VERTICAL);card.setPadding(dp(16),dp(8),dp(16),dp(16));card.setBackground(background(Color.WHITE,16));
        LinearLayout.LayoutParams box=new LinearLayout.LayoutParams(-1,-2);box.setMargins(0,dp(12),0,0);body.addView(card,box);
        TextView heading=label(card,title,19,INK);heading.setTypeface(null,Typeface.BOLD);if(!note.isEmpty())label(card,note,13,MUTED);return card;
    }
    private EditText capacity(LinearLayout body,String title,String hint){
        label(body,title,15,INK);EditText edit=new EditText(this);edit.setInputType(InputType.TYPE_CLASS_NUMBER);edit.setSingleLine(true);edit.setSelectAllOnFocus(true);edit.setFilters(new android.text.InputFilter[]{new android.text.InputFilter.LengthFilter(6)});
        edit.setTextSize(19);edit.setTextColor(INK);edit.setPadding(dp(12),0,dp(12),0);GradientDrawable field=background(0xfff3f5f7,9);field.setStroke(dp(1),0xffd8e0e5);edit.setBackground(field);body.addView(edit,new LinearLayout.LayoutParams(-1,dp(48)));label(body,hint,12,MUTED);return edit;
    }
    private Spinner choice(LinearLayout body,String title,String[] values){
        label(body,title,15,INK);Spinner spinner=new Spinner(this);spinner.setAdapter(new ArrayAdapter<>(this,android.R.layout.simple_spinner_dropdown_item,values));body.addView(spinner,new LinearLayout.LayoutParams(-1,dp(48)));return spinner;
    }
    private Button button(LinearLayout body,String text,Runnable click){Button b=new Button(this);b.setAllCaps(false);b.setText(text);b.setTextColor(ACCENT);b.setTextSize(15);b.setStateListAnimator(null);b.setElevation(0);b.setBackground(background(0xffeef3f5,10));b.setOnClickListener(v->click.run());LinearLayout.LayoutParams params=new LinearLayout.LayoutParams(-1,dp(48));params.setMargins(0,dp(5),0,dp(2));body.addView(b,params);return b;}
    @Override public void onCreate(Bundle state){
        super.onCreate(state);getWindow().setDecorFitsSystemWindows(false);
        LinearLayout root=new LinearLayout(this);root.setOrientation(LinearLayout.VERTICAL);root.setBackgroundColor(0xffedf1f4);
        root.setOnApplyWindowInsetsListener((view,insets)->{android.graphics.Insets bars=insets.getInsets(android.view.WindowInsets.Type.systemBars());int bottom=Math.max(bars.bottom,insets.getInsets(android.view.WindowInsets.Type.ime()).bottom);view.setPadding(bars.left,bars.top,bars.right,bottom);return insets;});
        ScrollView scroll=new ScrollView(this);scroll.setFillViewport(true);root.addView(scroll,new LinearLayout.LayoutParams(-1,0,1));
        LinearLayout body=new LinearLayout(this);body.setOrientation(LinearLayout.VERTICAL);body.setPadding(dp(16),dp(12),dp(16),dp(20));scroll.addView(body);
        TextView title=label(body,"潮汐拼音",27,INK);title.setTypeface(null,Typeface.BOLD);label(body,"键盘与学习设置",15,MUTED);
        LinearLayout keyboard=card(body,"键盘","键盘顶部点“键盘”选择九键或 26 键，点“设置”回到这里。");
        layoutChoice=choice(keyboard,"中文键盘",new String[]{"26 键全拼","九键拼音"});label(keyboard,"默认：九键拼音。已保存的选择会继续保留。",12,MUTED);
        LinearLayout assist=card(body,"输入与学习","只在本机保存词条和次数，密码输入不参与学习。");
        learning=new CheckBox(this);learning.setText("开启本地词条学习");assist.addView(learning);
        completion=new CheckBox(this);completion.setText("拼音、简拼与英文单词补全");assist.addView(completion);
        length=choice(assist,"自动学习长度",new String[]{"最多 4 个汉字（默认）","最多 8 个汉字","最多 16 个汉字"});
        LinearLayout capacity=card(body,"词库容量","每类 1–100000 条；低于已有条数时不会保存，也不会删词。");
        words=capacity(capacity,"自动学习词条上限","默认：8192 条");phrases=capacity(capacity,"个人词条上限","默认：4096 条");
        usage=label(capacity,"正在读取已用数量…",14,MUTED);
        LinearLayout manage=card(body,"数据与默认设置","恢复默认不会清除词条。已有数量超过默认容量时，会保留足够容量。");
        restore=button(manage,"恢复默认设置",this::restoreDefaults);
        clear=button(manage,"清除自动学习词条",()->new AlertDialog.Builder(this).setTitle("清除自动学习").setMessage("删除自动学习的词条和次数？个人词条和设置保留。").setNegativeButton("取消",null).setPositiveButton("清除",(d,w)->clearLearning()).show());
        LinearLayout system=card(body,"系统键盘管理","");button(system,"打开系统键盘设置",()->startActivity(new Intent(Settings.ACTION_INPUT_METHOD_SETTINGS)));
        button(system,"选择输入法",()->((android.view.inputmethod.InputMethodManager)getSystemService(INPUT_METHOD_SERVICE)).showInputMethodPicker());
        label(body,"本应用没有联网权限。升级请覆盖安装；卸载会删除手机中的个人数据。",13,MUTED);
        LinearLayout footer=new LinearLayout(this);footer.setOrientation(LinearLayout.VERTICAL);footer.setPadding(dp(16),dp(6),dp(16),dp(10));footer.setBackgroundColor(Color.WHITE);root.addView(footer,new LinearLayout.LayoutParams(-1,-2));
        LinearLayout feedbackRow=new LinearLayout(this);feedbackRow.setGravity(Gravity.CENTER_VERTICAL);footer.addView(feedbackRow);
        progress=new ProgressBar(this);feedbackRow.addView(progress,new LinearLayout.LayoutParams(dp(20),dp(20)));
        feedback=new TextView(this);feedback.setTextSize(14);feedback.setPadding(dp(8),dp(6),0,dp(6));feedback.setAccessibilityLiveRegion(View.ACCESSIBILITY_LIVE_REGION_POLITE);feedbackRow.addView(feedback,new LinearLayout.LayoutParams(0,-2,1));
        retry=button(footer,"重新读取设置",this::load);retry.setVisibility(View.GONE);
        save=button(footer,"保存设置",()->save(false));save.setTextColor(Color.WHITE);save.setBackgroundTintList(android.content.res.ColorStateList.valueOf(ACCENT));
        bind(KeyboardDefaults.state(0,0),KeyboardPreferences.nineKey(this));usage.setText("正在读取已用数量…");
        setContentView(root);watchChanges();busy(true,"正在读取已保存设置，当前显示默认值…");
    }
    @Override public void onResume(){super.onResume();load();}
    private boolean alive(){return !isFinishing()&&!isDestroyed();}
    private void busy(boolean value,String message){
        busyState=value;progress.setVisibility(value?View.VISIBLE:View.GONE);
        for(View v:new View[]{save,clear,restore,words,phrases,learning,completion,layoutChoice,length})v.setEnabled(!value&&loaded);
        save.setAlpha(value||!loaded?.55f:1f);save.setText(value?"正在处理…":"保存设置");feedback.setTextColor(MUTED);feedback.setText(message);
    }
    private void bind(int[] state,boolean nine){
        binding=true;learning.setChecked(state[0]!=0);completion.setChecked(state[1]!=0);
        words.setText(getString(R.string.capacity_number,state[2]));phrases.setText(getString(R.string.capacity_number,state[3]));words.setError(null);phrases.setError(null);
        usage.setText(getString(R.string.capacity_usage,state[4],state[2],state[5],state[3]));length.setSelection(state[6]==16?2:state[6]==8?1:0);layoutChoice.setSelection(nine?1:0);binding=false;
    }
    private void show(int[] state){savedState=state.clone();savedNine=KeyboardPreferences.nineKey(this);bind(state,savedNine);}
    private void watchChanges(){
        TextWatcher watcher=new TextWatcher(){public void beforeTextChanged(CharSequence s,int a,int c,int f){}public void onTextChanged(CharSequence s,int a,int b,int c){dirty();}public void afterTextChanged(Editable e){}};
        words.addTextChangedListener(watcher);phrases.addTextChangedListener(watcher);learning.setOnCheckedChangeListener((v,c)->dirty());completion.setOnCheckedChangeListener((v,c)->dirty());
        AdapterView.OnItemSelectedListener selected=new AdapterView.OnItemSelectedListener(){public void onItemSelected(AdapterView<?> parent,View view,int position,long id){dirty();}public void onNothingSelected(AdapterView<?> parent){}};
        layoutChoice.setOnItemSelectedListener(selected);length.setOnItemSelectedListener(selected);
    }
    private boolean numberMatches(EditText field,int value){try{return Integer.parseInt(field.getText().toString())==value;}catch(NumberFormatException error){return false;}}
    private void dirty(){
        if(binding||busyState||!loaded||savedState==null)return;
        boolean changed=!numberMatches(words,savedState[2])||!numberMatches(phrases,savedState[3])||learning.isChecked()!=(savedState[0]!=0)||completion.isChecked()!=(savedState[1]!=0)||(layoutChoice.getSelectedItemPosition()==1)!=savedNine||(length.getSelectedItemPosition()==2?16:length.getSelectedItemPosition()==1?8:4)!=savedState[6];
        if(changed){save.setText("保存设置");feedback.setTextColor(MUTED);feedback.setText("有未保存的修改");}else if("有未保存的修改".contentEquals(feedback.getText())){save.setText("✓ 已保存");feedback.setTextColor(ACCENT);feedback.setText("与已保存设置一致");}
    }
    private void notice(String text,boolean success){
        feedback.setTextColor(success?ACCENT:0xffb33b32);feedback.setText(text);Toast.makeText(this,text,success?Toast.LENGTH_SHORT:Toast.LENGTH_LONG).show();
    }
    private void load(){
        loaded=false;retry.setVisibility(View.GONE);busy(true,"正在读取已保存设置…");
        EngineRuntime.worker.execute(()->{
            try{EngineRuntime.ensure(getApplicationContext());int[] state=EngineBridge.settings(0,0,false,false,4,false);
                main.post(()->{if(alive()){loaded=true;show(state);busy(false,"设置已加载，修改后点击下方保存。");}});
            }catch(Exception|LinkageError error){main.post(()->{if(alive()){busy(false,"读取失败，请点击重新读取。");retry.setVisibility(View.VISIBLE);notice("设置读取失败，尚未修改任何数据。",false);}});}
        });
    }
    private int capacity(EditText field){
        try{int value=Integer.parseInt(field.getText().toString());if(value>=1&&value<=100000)return value;}catch(NumberFormatException ignored){}
        field.setError(getString(R.string.capacity_invalid));field.requestFocus();return -1;
    }
    private void save(boolean restoring){
        if(!loaded||busyState)return;
        final int wordLimit=capacity(words);if(wordLimit<0)return;final int phraseLimit=capacity(phrases);if(phraseLimit<0)return;
        if(savedState!=null&&(wordLimit<savedState[4]||phraseLimit<savedState[5])){notice("容量不能低于当前已用数量，未删除任何词条。",false);return;}
        final boolean learn=learning.isChecked(),complete=completion.isChecked(),nine=layoutChoice.getSelectedItemPosition()==1;
        final int maxLength=length.getSelectedItemPosition()==2?16:length.getSelectedItemPosition()==1?8:4;
        View focus=getCurrentFocus();if(focus!=null)((android.view.inputmethod.InputMethodManager)getSystemService(INPUT_METHOD_SERVICE)).hideSoftInputFromWindow(focus.getWindowToken(),0);
        busy(true,"正在保存，请稍候…");save.setText("正在保存…");
        EngineRuntime.worker.execute(()->{
            try{EngineRuntime.ensure(getApplicationContext());int[] state=EngineBridge.settings(wordLimit,phraseLimit,learn,complete,maxLength,true);
                boolean layoutSaved=KeyboardPreferences.nineKey(getApplicationContext(),nine);
                main.post(()->{if(alive()){
                    show(state);busy(false,"");
                    if(layoutSaved){save.setText("✓ 已保存");notice(restoring?"默认设置已恢复并保存，词条保留。":"设置已保存，返回输入框即可使用。",true);}
                    else notice("词库设置已保存，但布局未能保存，请重试。",false);
                }});
            }catch(Exception|LinkageError error){main.post(()->{if(alive()){busy(false,"");notice("保存失败，原有词库设置保留。请检查容量或存储空间后重试。",false);}});}
        });
    }
    private void restoreDefaults(){
        if(!loaded||busyState)return;
        new AlertDialog.Builder(this).setTitle("恢复默认设置？").setMessage("九键拼音；开启学习与补全；最多学习 4 个汉字；容量为 8192 / 4096 条。\n\n不会删除词条；已有数量超过默认容量时会保留足够容量。").setNegativeButton("取消",null).setPositiveButton("恢复并保存",(dialog,which)->{bind(KeyboardDefaults.state(savedState[4],savedState[5]),KeyboardDefaults.NINE_KEY);save(true);}).show();
    }
    private void clearLearning(){
        busy(true,"正在清除自动学习…");EngineRuntime.worker.execute(()->{
            try{EngineRuntime.ensure(getApplicationContext());EngineBridge.request(6,0,0);int[] state=EngineBridge.settings(0,0,false,false,4,false);
                main.post(()->{if(alive()){show(state);busy(false,"");notice("自动学习已清除，个人词条保留。",true);}});
            }catch(Exception|LinkageError error){main.post(()->{if(alive()){busy(false,"");notice("清除失败，请重试。",false);}});}
        });
    }
}
