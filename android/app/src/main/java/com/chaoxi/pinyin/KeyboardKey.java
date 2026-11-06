package com.chaoxi.pinyin;

import android.content.Context;
import android.graphics.*;
import android.view.MotionEvent;
import android.view.View;

/** A key without the platform Button's inset, capitalization or elevation. */
// This key is created with an explicit action and is never inflated from XML.
@android.annotation.SuppressLint("ViewConstructor")
final class KeyboardKey extends View {
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final RectF box=new RectF();
    private String label,hint,action;
    private final int style;
    private boolean accent;
    private Runnable repeat;
    private final float density,scaled;
    KeyboardKey(Context context,String action,String label,String hint,int style,Runnable click){
        super(context);this.action=action;this.label=label;this.hint=hint;this.style=style;
        density=getResources().getDisplayMetrics().density;scaled=getResources().getDisplayMetrics().scaledDensity;
        setClickable(true);setFocusable(true);setLabel(label);setOnClickListener(v->click.run());
    }
    void setLabel(String value){label=value;setContentDescription(hint.isEmpty()?value:hint+" "+value);requestLayout();invalidate();}
    void accent(boolean value){accent=value;invalidate();}
    @Override protected void onMeasure(int widthSpec,int heightSpec){
        paint.setTextSize(18*scaled);
        int desired=(int)Math.max(48*density,paint.measureText(label)+28*density);
        setMeasuredDimension(resolveSize(desired,widthSpec),resolveSize((int)(44*density),heightSpec));
    }
    @Override protected void onDraw(Canvas canvas){
        super.onDraw(canvas);
        float w=getWidth(),h=getHeight();boolean flat=style==KeyboardLayout.FLAT||style==KeyboardLayout.CANDIDATE;
        box.set(density/2,density/2,w-density/2,h-density);
        if(!flat){
            paint.setColor(0xffcbd5cb);box.offset(0,density);canvas.drawRoundRect(box,5*density,5*density,paint);box.offset(0,-density);
            paint.setColor(isPressed()?0xffcee7d4:style==KeyboardLayout.FUNCTION?0xffe4ece5:0xfffcfcfd);
            canvas.drawRoundRect(box,5*density,5*density,paint);
        }else if(isPressed()){
            paint.setColor(0xffdcebdd);canvas.drawRoundRect(box,5*density,5*density,paint);
        }
        int ink=accent?0xff247983:0xff24282f;
        if(action.equals("⌫")){drawDelete(canvas,w,h,ink);return;}
        if(action.equals("↵")){drawEnter(canvas,w,h,ink);return;}
        if(action.equals("hide")){drawChevron(canvas,w,h,ink);return;}
        paint.setStyle(Paint.Style.FILL);paint.setTypeface(Typeface.create("sans-serif",accent?Typeface.BOLD:Typeface.NORMAL));
        float base=style==KeyboardLayout.PUNCTUATION?29:flat?(style==KeyboardLayout.CANDIDATE?19:14):hint.isEmpty()?(label.length()>3?17:23):(label.equals("分词")?19:26);
        float size=Math.min(base*scaled,h*(style==KeyboardLayout.PUNCTUATION?.80f:hint.isEmpty()?.58f:.43f));
        paint.setTextSize(size);
        float available=Math.max(1,w-(flat?12:10)*density);
        if(paint.measureText(label)>available)paint.setTextSize(size*available/paint.measureText(label));
        paint.setColor(ink);paint.setTextAlign(Paint.Align.CENTER);
        Paint.FontMetrics fm=paint.getFontMetrics();float middle=hint.isEmpty()?h*.50f:h*.61f;
        canvas.drawText(label,w/2,middle-(fm.ascent+fm.descent)/2,paint);
        if(!hint.isEmpty()){
            paint.setTypeface(Typeface.create("sans-serif",Typeface.NORMAL));paint.setTextSize(Math.min(12*scaled,h*.23f));paint.setColor(0xff858d98);
            fm=paint.getFontMetrics();canvas.drawText(hint,w/2,h*.20f-(fm.ascent+fm.descent)/2,paint);
        }
    }
    private void stroke(int color){paint.setColor(color);paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(1.6f*density);paint.setStrokeCap(Paint.Cap.ROUND);paint.setStrokeJoin(Paint.Join.ROUND);}
    private void drawDelete(Canvas canvas,float w,float h,int color){
        stroke(color);float x=w/2,y=h/2,s=Math.min(11*density,w*.30f);Path path=new Path();
        path.moveTo(x-s,y);path.lineTo(x-s*.35f,y-s*.65f);path.lineTo(x+s,y-s*.65f);path.lineTo(x+s,y+s*.65f);path.lineTo(x-s*.35f,y+s*.65f);path.close();canvas.drawPath(path,paint);
        canvas.drawLine(x-s*.08f,y-s*.25f,x+s*.43f,y+s*.25f,paint);canvas.drawLine(x-s*.08f,y+s*.25f,x+s*.43f,y-s*.25f,paint);paint.setStyle(Paint.Style.FILL);
    }
    private void drawEnter(Canvas canvas,float w,float h,int color){
        stroke(color);float x=w/2,y=h/2,s=Math.min(11*density,w*.30f);Path p=new Path();p.moveTo(x+s,y-s*.5f);p.lineTo(x+s,y+s*.25f);p.lineTo(x-s,y+s*.25f);p.lineTo(x-s*.45f,y-s*.3f);p.moveTo(x-s,y+s*.25f);p.lineTo(x-s*.45f,y+s*.8f);canvas.drawPath(p,paint);paint.setStyle(Paint.Style.FILL);
    }
    private void drawChevron(Canvas canvas,float w,float h,int color){stroke(color);float s=6*density;canvas.drawLine(w/2-s,h/2-s/2,w/2,h/2+s/2,paint);canvas.drawLine(w/2,h/2+s/2,w/2+s,h/2-s/2,paint);paint.setStyle(Paint.Style.FILL);}
    @Override public boolean performClick(){super.performClick();return true;}
    void cancelTouch(){if(repeat!=null)removeCallbacks(repeat);setPressed(false);invalidate();}
    @Override protected void onDetachedFromWindow(){cancelTouch();super.onDetachedFromWindow();}
    @Override public boolean onTouchEvent(MotionEvent event){
        if(!isEnabled())return false;
        switch(event.getActionMasked()){
            case MotionEvent.ACTION_DOWN:
                setPressed(true);invalidate();
                if(action.equals("⌫")){performClick();repeat=new Runnable(){public void run(){if(isPressed()&&isAttachedToWindow()){performClick();postDelayed(this,85);}}};postDelayed(repeat,400);}return true;
            case MotionEvent.ACTION_MOVE:
                if(event.getX()<0||event.getY()<0||event.getX()>getWidth()||event.getY()>getHeight())cancelTouch();return true;
            case MotionEvent.ACTION_UP:
                boolean click=isPressed()&&!action.equals("⌫");cancelTouch();if(click)performClick();return true;
            case MotionEvent.ACTION_CANCEL:cancelTouch();return true;
            default:return true;
        }
    }
}
