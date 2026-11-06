package com.chaoxi.pinyin;

import android.inputmethodservice.InputMethodService;
import android.text.InputType;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputConnection;
import android.widget.*;
import android.os.Handler;
import android.os.Looper;

public final class ChaoxiInputService extends InputMethodService {
    private final Handler main = new Handler(Looper.getMainLooper());
    private LinearLayout panel, candidates, candidateBar;
    private GridLayout expanded;
    private ScrollView expandedScroll;
    private KeyboardKey previousButton,nextButton,moreButton;
    private boolean expandedCandidates;
    private KeyboardPanel keys;
    private HorizontalScrollView candidateScroll;
    private TextView status;
    private KeyboardKey layoutButton;
    private boolean numericPage;
    private boolean nineKey;
    private boolean english, symbols, secure, ready, active, privateInput;
    private volatile int generation;
    private int pending;
    private boolean uppercase;
    private String preedit = "";
    private boolean prediction;
    private int candidateCount;
    private static final int CLEAR = 2, KEY = 1;
    private int dp(int value) { return Math.round(value * getResources().getDisplayMetrics().density); }
    @Override public View onCreateInputView() {
        panel=new LinearLayout(this);panel.setOrientation(LinearLayout.VERTICAL);panel.setBackgroundColor(0xffd5d9df);
        status=new TextView(this);status.setTextSize(13);status.setTextColor(0xff45515c);status.setSingleLine(true);status.setPadding(dp(12),dp(2),dp(12),0);panel.addView(status,new LinearLayout.LayoutParams(-1,dp(24)));
        LinearLayout toolbar=new LinearLayout(this);toolbar.setGravity(android.view.Gravity.CENTER_VERTICAL);toolbar.setPadding(dp(4),0,dp(4),0);
        View spacer=new View(this);toolbar.addView(spacer,new LinearLayout.LayoutParams(0,dp(1),1));
        layoutButton=flat("layout","键盘",this::showLayouts);toolbar.addView(layoutButton,new LinearLayout.LayoutParams(dp(45),dp(42)));
        toolbar.addView(flat("settings","设置",this::openSettings),new LinearLayout.LayoutParams(dp(45),dp(42)));
        toolbar.addView(flat("switch","切换",()->switchToNextInputMethod(false)),new LinearLayout.LayoutParams(dp(43),dp(42)));
        toolbar.addView(flat("hide","收起",()->requestHideSelf(0)),new LinearLayout.LayoutParams(dp(36),dp(42)));panel.addView(toolbar);
        candidateBar=new LinearLayout(this);candidateBar.setGravity(android.view.Gravity.CENTER_VERTICAL);
        candidateScroll=new HorizontalScrollView(this);candidateScroll.setHorizontalScrollBarEnabled(false);candidateScroll.setFillViewport(false);
        candidates=new LinearLayout(this);candidates.setGravity(android.view.Gravity.CENTER_VERTICAL);candidateScroll.addView(candidates);candidateBar.addView(candidateScroll,new LinearLayout.LayoutParams(0,dp(44),1));
        previousButton=flat("previous","‹",()->request(KEY,0xff55));nextButton=flat("next","›",()->request(KEY,0xff56));
        moreButton=flat("more","展开",()->{expandedCandidates=!expandedCandidates;showCandidatePanel();});
        candidateBar.addView(previousButton,new LinearLayout.LayoutParams(dp(32),dp(44)));candidateBar.addView(nextButton,new LinearLayout.LayoutParams(dp(32),dp(44)));candidateBar.addView(moreButton,new LinearLayout.LayoutParams(dp(42),dp(44)));
        panel.addView(candidateBar,new LinearLayout.LayoutParams(-1,dp(44)));candidateBar.setVisibility(View.GONE);
        expandedScroll=new ScrollView(this);expanded=new GridLayout(this);expanded.setColumnCount(3);expandedScroll.addView(expanded);panel.addView(expandedScroll,new LinearLayout.LayoutParams(-1,dp(220)));expandedScroll.setVisibility(View.GONE);
        keys=new KeyboardPanel(this,this::press);panel.addView(keys);
        buildKeys();
        panel.setOnApplyWindowInsetsListener((view,insets)->{android.graphics.Insets bars=insets.getInsets(android.view.WindowInsets.Type.navigationBars());view.setPadding(bars.left,0,bars.right,bars.bottom);return insets;});
        prepare();return panel;
    }
    private KeyboardKey flat(String action,String label,Runnable click){return new KeyboardKey(this,action,label,"",KeyboardLayout.FLAT,click);}
    private void openSettings(){startActivity(new android.content.Intent(this,SettingsActivity.class).addFlags(android.content.Intent.FLAG_ACTIVITY_NEW_TASK));}
    private void showLayouts(){
        if(secure)return;
        PopupMenu menu=new PopupMenu(this,layoutButton);
        menu.getMenu().add("九键拼音").setOnMenuItemClickListener(item->{changeLayout(true);return true;});
        menu.getMenu().add("26 键全拼").setOnMenuItemClickListener(item->{changeLayout(false);return true;});menu.show();
    }
    private void prepare() {
        status.setVisibility(View.VISIBLE);status.setText("正在准备词库…");
        final int token = generation;
        final boolean noLearning=privateInput;
        final boolean layout=nineKey;
        EngineRuntime.worker.execute(() -> {
            try {
                EngineRuntime.ensure(getApplicationContext());
                EngineBridge.privacy(noLearning);
                EngineBridge.setNineKey(layout);
                EngineBridge.request(CLEAR,0,0);
                main.post(() -> { if(token!=generation)return; ready=true; render(null); });
            } catch (Exception | LinkageError error) {
                main.post(() -> { if(token!=generation)return; ready=false; status.setText("词库准备失败，点击重试"); status.setOnClickListener(v -> prepare()); });
            }
        });
    }
    @Override public void onStartInput(EditorInfo info, boolean restarting) {
        super.onStartInput(info,restarting);if(keys!=null)keys.cancelTouches(); generation++; active=true; ready=false;pending=0;uppercase=false;
        preedit="";prediction=false;candidateCount=0;expandedCandidates=false;symbols=false;numericPage=false;english=false;nineKey=KeyboardPreferences.nineKey(this);
        int kind=info.inputType & InputType.TYPE_MASK_CLASS;
        int variation=info.inputType & InputType.TYPE_MASK_VARIATION;
        secure=(kind==InputType.TYPE_CLASS_TEXT && (variation==InputType.TYPE_TEXT_VARIATION_PASSWORD || variation==InputType.TYPE_TEXT_VARIATION_VISIBLE_PASSWORD || variation==InputType.TYPE_TEXT_VARIATION_WEB_PASSWORD)) || (kind==InputType.TYPE_CLASS_NUMBER && variation==InputType.TYPE_NUMBER_VARIATION_PASSWORD);
        privateInput=secure || (info.imeOptions & EditorInfo.IME_FLAG_NO_PERSONALIZED_LEARNING)!=0;
        english=secure || kind==InputType.TYPE_CLASS_NUMBER || kind==InputType.TYPE_CLASS_PHONE;
        if(panel!=null){buildKeys();prepare();}
    }
    @Override public void onFinishInput() {
        if(keys!=null)keys.cancelTouches();active=false;generation++;ready=false;preedit="";candidateCount=0;prediction=false;
        EngineRuntime.worker.execute(() -> { try {EngineBridge.request(CLEAR,0,0);} catch(Exception | LinkageError ignored) {} });
        super.onFinishInput();
    }
    @Override public boolean onEvaluateFullscreenMode() { return false; }
    private void request(int op,int key) { request(op,key,null,null); }
    private void request(int op,int key,Runnable after) {request(op,key,after,null);}
    private void request(int op,int key,Runnable after,Runnable fallback) {
        transact(()->EngineBridge.request(op,key,0),after,fallback);
    }
    private void transact(java.util.function.Supplier<String[]> operation,Runnable after,Runnable fallback) {
        if(!ready || !active)return;
        final int token=generation;final InputConnection connection=getCurrentInputConnection();
        if(connection==null)return;
        pending++;candidates.removeAllViews();expanded.removeAllViews();previousButton.setEnabled(false);nextButton.setEnabled(false);moreButton.setEnabled(false);
        EngineRuntime.worker.execute(() -> {
            try {
                if(token!=generation)return;
                EngineState state=new EngineState(operation.get());
                main.post(() -> {
                    if(token!=generation || !active || connection!=getCurrentInputConnection())return;
                    pending--;
                    connection.beginBatchEdit();
                    try {
                        if(!state.commit.isEmpty())connection.commitText(state.commit,1);
                        if(!state.preedit.isEmpty())connection.setComposingText(state.preedit,1);
                        else if(!preedit.isEmpty() && state.commit.isEmpty())connection.setComposingText("",1);
                        if(state.preedit.isEmpty())connection.finishComposingText();
                    } finally {connection.endBatchEdit();}
                    render(state);if(!state.eaten && fallback!=null)fallback.run();if(after!=null)after.run();
                });
            } catch(Exception | LinkageError error) {
                main.post(() -> {if(token==generation){ready=false;status.setVisibility(View.VISIBLE);status.setText("输入引擎异常，请切换键盘后重试");}});
            }
        });
    }
    @Override public void onUpdateSelection(int oldStart,int oldEnd,int start,int end,int composingStart,int composingEnd) {
        super.onUpdateSelection(oldStart,oldEnd,start,end,composingStart,composingEnd);
        if(!preedit.isEmpty() && composingStart>=0 && (start!=composingEnd || end!=composingEnd)) {
            InputConnection connection=getCurrentInputConnection();if(connection!=null)connection.finishComposingText();
            generation++;pending=0;preedit="";candidateCount=0;prediction=false;ready=false;prepare();
        }
    }
    private void render(EngineState state) {
        candidates.removeAllViews();expanded.removeAllViews();
        if(state!=null){
            preedit=state.preedit;prediction=state.prediction;candidateCount=state.candidates.length;
            for(int i=0;pending==0&&i<state.candidates.length;i++){
                final int index=i;
                KeyboardKey key=candidateKey(state.candidates[i],index);key.accent(index==0);
                LinearLayout.LayoutParams spacing=new LinearLayout.LayoutParams(-2,dp(42));spacing.setMargins(dp(3),0,dp(3),0);candidates.addView(key,spacing);
                View divider=new View(this);divider.setBackgroundColor(0xffbec6cf);candidates.addView(divider,new LinearLayout.LayoutParams(dp(1),dp(16)));
                KeyboardKey tile=candidateKey(state.candidates[i],index);tile.accent(index==0);
                GridLayout.LayoutParams cell=new GridLayout.LayoutParams(GridLayout.spec(i/3),GridLayout.spec(i%3,1f));cell.width=0;cell.height=dp(56);cell.setMargins(dp(3),dp(3),dp(3),dp(3));expanded.addView(tile,cell);
            }
            previousButton.setEnabled(pending==0&&state.page>0);nextButton.setEnabled(pending==0&&!state.last);moreButton.setEnabled(pending==0&&candidateCount>0);
            previousButton.setAlpha(previousButton.isEnabled()?1f:.3f);nextButton.setAlpha(nextButton.isEnabled()?1f:.3f);
            if(pending==0)keys.setSpellings(state.spellings,state.prefix);
            if(pending==0){candidateScroll.scrollTo(0,0);expandedScroll.scrollTo(0,0);}
        }else keys.setSpellings(new String[0],"");
        candidateBar.setVisibility(candidateCount>0||prediction?View.VISIBLE:View.GONE);
        if(preedit.isEmpty()&&!prediction)expandedCandidates=false;
        showCandidatePanel();
        status.setOnClickListener(null);status.setVisibility(!ready||!preedit.isEmpty()?View.VISIBLE:View.GONE);
        status.setText(!ready?"正在准备词库…":preedit);
        layoutButton.setEnabled(!secure);
    }
    private KeyboardKey candidateKey(String text,int index){return new KeyboardKey(this,"candidate",text,"",KeyboardLayout.CANDIDATE,()->{if(pending==0)request(KEY,'1'+index);});}
    private void showCandidatePanel(){
        boolean show=expandedCandidates&&candidateCount>0;
        keys.setVisibility(show?View.GONE:View.VISIBLE);expandedScroll.setVisibility(show?View.VISIBLE:View.GONE);moreButton.setLabel(show?"收起":"展开");
    }
    private void buildKeys(){
        keys.update(nineKey,symbols?(numericPage?KeyboardLayout.NUMBERS:KeyboardLayout.SYMBOLS):KeyboardLayout.LETTERS,english||secure,uppercase);
        if(layoutButton!=null)layoutButton.setEnabled(!secure);
    }
    private void changeLayout(boolean value) {
        if(secure || !ready)return;
        request(KEY,0xff0d);ready=false;
        final int token=generation;nineKey=value;english=false;uppercase=false;symbols=false;numericPage=false;
        EngineRuntime.worker.execute(() -> {
            try {EngineBridge.setNineKey(value);EngineBridge.request(CLEAR,0,0);boolean remembered=KeyboardPreferences.nineKey(getApplicationContext(),value);
                main.post(() -> {if(token==generation){pending=0;preedit="";prediction=false;candidateCount=0;ready=true;buildKeys();render(null);if(!remembered)Toast.makeText(this,"布局已切换，但未能记住选择。",Toast.LENGTH_LONG).show();}});
            }catch(Exception | LinkageError error){main.post(() -> {if(token==generation){status.setVisibility(View.VISIBLE);status.setText("键盘切换失败，请切换键盘后重试");}});}
        });
    }
    private void enqueueAction(Runnable action) {
        final int token=generation;final InputConnection connection=getCurrentInputConnection();
        EngineRuntime.worker.execute(() -> main.post(() -> {if(active && token==generation && connection!=null && connection==getCurrentInputConnection())action.run();}));
    }
    private void direct(String text) {InputConnection c=getCurrentInputConnection();if(c!=null)c.commitText(text,1);}
    private void press(String key) {
        if(key.startsWith("pinyin:")){if(pending==0&&nineKey&&!english&&!secure)transact(()->EngineBridge.chooseSpelling(key.substring(7)),null,null);return;}
        if(key.equals("⇧")){if(!english && !secure){request(KEY,0xff0d);english=true;uppercase=true;}else uppercase=!uppercase;buildKeys();return;}
        if(key.equals("🌐")){switchToNextInputMethod(false);return;}
        if(key.equals("中") || key.equals("英")) {
            if(secure)return;
            request(KEY,0xff0d);english=!english;if(!english)uppercase=false;buildKeys();return;
        }
        if(key.equals("符")){symbols=true;numericPage=false;buildKeys();return;}
        if(key.equals("123")){symbols=true;numericPage=true;buildKeys();return;}
        if(key.equals("ABC")){symbols=false;numericPage=false;buildKeys();return;}
        if(key.equals("重输")){if(ready)request(CLEAR,0);return;}
        if(key.equals("⌫")){
            Runnable erase=() -> {InputConnection c=getCurrentInputConnection();if(c!=null)c.deleteSurroundingTextInCodePoints(1,0);};
            if(english || secure)enqueueAction(erase);else request(KEY,0xff08,null,erase);return;
        }
        if(key.equals("空格")){
            if(english || secure)enqueueAction(() -> direct(" "));else request(KEY,' ',null,() -> direct(" "));return;
        }
        if(key.equals("↵")){
            Runnable enter=() -> {
                EditorInfo info=getCurrentInputEditorInfo();InputConnection c=getCurrentInputConnection();if(c==null || info==null)return;
                int action=info.imeOptions & EditorInfo.IME_MASK_ACTION;
                if((info.imeOptions & EditorInfo.IME_FLAG_NO_ENTER_ACTION)==0 && action!=EditorInfo.IME_ACTION_NONE && action!=EditorInfo.IME_ACTION_UNSPECIFIED)c.performEditorAction(action);
                else direct("\n");
            };
            if(english || secure)enqueueAction(enter);else request(KEY,' ',null,enter);return;
        }
        if(english || secure){enqueueAction(() -> direct(key));return;}
        if(!ready)return;
        if(nineKey&&!symbols&&key.equals("0")){request(KEY,' ',()->direct("0"));return;}
        if(nineKey && !symbols && key.length()>1 && key.charAt(0)>='1' && key.charAt(0)<='9'){
            if(key.charAt(0)=='1'){if(!preedit.isEmpty()||pending>0)request(KEY,'\'');}else request(19,key.charAt(0));return;
        }
        // Digits select candidates only through candidate taps; symbols commit after the first choice.
        if(symbols)request(KEY,' ',() -> direct(key));
        else request(KEY,key.charAt(0));
    }
}
