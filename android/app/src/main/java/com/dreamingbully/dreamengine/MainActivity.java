package com.dreamingbully.dreamengine;

import android.app.Activity;
import android.os.Bundle;
import android.os.Build;
import android.os.Handler;
import android.os.Looper;
import android.content.Intent;
import android.net.Uri;
import android.graphics.*;
import android.graphics.drawable.ColorDrawable;
import android.widget.FrameLayout;
import android.view.*;
import android.view.TextureView.SurfaceTextureListener;
import java.io.*;

/**
 * Android-native DreamEngine editor shell.
 * The UI is deliberately drawn as an editor, not as a vertical collection of demo buttons.
 * Editor state remains in C++ EditorCore; this class owns Android presentation/input only.
 */
public final class MainActivity extends Activity {
    static { System.loadLibrary("dream_engine_android"); }
    private static final int PICK_MODULE_DIR = 7001;

    private static native void nativeStart(String moduleDir, String pythonHome);
    private static native void nativeStop();
    private static native void nativeUpdate(double dt);
    private static native void nativeReload();
    private static native String nativeDrainLog();
    private static native void nativeSurfaceChanged(android.view.Surface surface, int w, int h);
    private static native void nativeSurfaceDestroyed();
    private static native void nativeRender(double dt);
    private static native String nativeEditorHierarchy();
    private static native String nativeEditorInspector();
    private static native int nativeEditorCreate(String kind);
    private static native boolean nativeEditorSelect(int index);
    private static native boolean nativeEditorDelete();
    private static native boolean nativeEditorSave(String path);
    private static native void nativeEditorSetPlaying(boolean playing);
    private static native boolean nativeEditorIsPlaying();
    private static native boolean nativeEditorMove(float dx, float dy, float dz);

    private final Handler tickHandler = new Handler(Looper.getMainLooper());
    private long lastTickNanos;
    private File moduleDir;
    private EditorView editorView;
    private String logText = "";

    private final Runnable tick = new Runnable() {
        @Override public void run() {
            if (isFinishing()) return;
            long now = System.nanoTime();
            double dt = lastTickNanos == 0 ? 1.0 / 60.0 : Math.min((now - lastTickNanos) * 1.0e-9, 0.25);
            lastTickNanos = now;
            nativeUpdate(dt);
            nativeRender(dt);
            if (editorView != null) {
                editorView.invalidate();
                if ((now / 1_000_000L) % 500 < 20) editorView.refreshData();
            }
            tickHandler.postDelayed(this, 16);
        }
    };

    @Override protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        getWindow().setStatusBarColor(Color.rgb(25, 25, 28));
        getWindow().setNavigationBarColor(Color.rgb(18, 18, 20));
        moduleDir = new File(getFilesDir(), "modules");
        if (!moduleDir.exists()) moduleDir.mkdirs();
        copyBundledModules();

        String abi = Build.SUPPORTED_ABIS.length > 0 ? Build.SUPPORTED_ABIS[0] : "arm64-v8a";
        File pythonHome = new File(getFilesDir(), "python/" + abi);
        copyBundledPython(abi, pythonHome);

        FrameLayout root = new FrameLayout(this);
        root.setBackgroundColor(Color.rgb(30, 30, 32));

        TextureView viewport = new TextureView(this);
        viewport.setOpaque(true);
        viewport.setSurfaceTextureListener(new SurfaceTextureListener() {
            @Override public void onSurfaceTextureAvailable(SurfaceTexture st, int w, int h) {
                nativeSurfaceChanged(new Surface(st), w, h);
            }
            @Override public void onSurfaceTextureSizeChanged(SurfaceTexture st, int w, int h) {
                nativeSurfaceChanged(new Surface(st), w, h);
            }
            @Override public boolean onSurfaceTextureDestroyed(SurfaceTexture st) {
                nativeSurfaceDestroyed();
                return true;
            }
            @Override public void onSurfaceTextureUpdated(SurfaceTexture st) {}
        });
        root.addView(viewport, new FrameLayout.LayoutParams(-1, -1));

        editorView = new EditorView();
        root.addView(editorView, new FrameLayout.LayoutParams(-1, -1));
        setContentView(root);

        nativeStart(moduleDir.getAbsolutePath(), pythonHome.getAbsolutePath());
        appendLog(nativeDrainLog());
        editorView.refreshData();
        lastTickNanos = System.nanoTime();
        tickHandler.post(tick);
    }

    private void appendLog(String s) {
        if (s == null || s.isEmpty()) return;
        logText += s;
        if (logText.length() > 14000) logText = logText.substring(logText.length() - 14000);
    }

    private void importModules() {
        Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        startActivityForResult(i, PICK_MODULE_DIR);
    }

    private void saveScene() {
        File sceneDir = new File(getFilesDir(), "project/scenes");
        sceneDir.mkdirs();
        boolean ok = nativeEditorSave(new File(sceneDir, "main.scene").getAbsolutePath());
        appendLog(ok ? "Editor: scene saved\n" : "Editor: scene save failed\n");
    }

    private final class EditorView extends View {
        private final Paint p = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final Paint stroke = new Paint(Paint.ANTI_ALIAS_FLAG);
        private String hierarchy = "", inspector = "";
        private int selectedIndex = -1;
        private float downX, downY;
        private boolean draggingViewport;
        private int tool = 0; // 0 select, 1 move, 2 rotate, 3 scale
        private int bottomTab = 0; // project / console / profiler
        private int pressedAction = -1;

        private final int topH = 64;
        private final int toolbarH = 52;
        private final int leftW = 280;
        private final int rightW = 330;
        private final int bottomH = 150;

        EditorView() {
            super(MainActivity.this);
            p.setTypeface(Typeface.create("sans", Typeface.NORMAL));
            stroke.setStyle(Paint.Style.STROKE);
            setFocusable(true);
        }

        void refreshData() {
            hierarchy = nativeEditorHierarchy();
            inspector = nativeEditorInspector();
            String[] lines = hierarchy.split("\\n");
            selectedIndex = -1;
            for (String line : lines) {
                int star = line.indexOf('*');
                if (star >= 0) {
                    try { selectedIndex = Integer.parseInt(line.substring(0, star).trim()); } catch (Exception ignored) {}
                }
            }
            invalidate();
        }

        private void fill(Canvas c, int color, float l, float t, float r, float b) {
            p.setStyle(Paint.Style.FILL); p.setColor(color); c.drawRect(l,t,r,b,p);
        }
        private void text(Canvas c, String s, float x, float y, float size, int color) {
            p.setStyle(Paint.Style.FILL); p.setTypeface(Typeface.create("sans", Typeface.NORMAL)); p.setTextSize(size); p.setColor(color); c.drawText(s,x,y,p);
        }
        private void mono(Canvas c, String s, float x, float y, float size, int color) {
            p.setStyle(Paint.Style.FILL); p.setTypeface(Typeface.MONOSPACE); p.setTextSize(size); p.setColor(color); c.drawText(s,x,y,p);
        }
        private boolean hit(float x,float y,float l,float t,float r,float b){return x>=l&&x<=r&&y>=t&&y<=b;}

        @Override protected void onDraw(Canvas c) {
            super.onDraw(c);
            final int w=getWidth(), h=getHeight();
            // Opaque editor chrome. Center viewport is intentionally left transparent so TextureView shows through.
            fill(c, Color.rgb(35,35,38), 0,0,w,topH);
            fill(c, Color.rgb(42,42,45), 0,topH,w,topH+toolbarH);
            fill(c, Color.rgb(38,38,41), 0,topH+toolbarH,leftW,h-bottomH);
            fill(c, Color.rgb(38,38,41), w-rightW,topH+toolbarH,w,h-bottomH);
            fill(c, Color.rgb(34,34,37), 0,h-bottomH,w,h);

            // Menu bar
            text(c,"DreamEngine",18,40,21,Color.WHITE);
            String[] menu={"File","Edit","Assets","GameObject","Component","Window","Help"};
            float mx=170;
            for(String m:menu){ text(c,m,mx,39,14,Color.rgb(215,215,218)); mx+=Math.max(58,p.measureText(m)+24); }
            text(c,"SCENE: main.scene",w-170,39,12,Color.rgb(150,150,155));

            // Toolbar
            button(c,"▶",18,topH+8,52,topH+44,Color.rgb(58,108,72));
            button(c,"■",76,topH+8,52,topH+44,Color.rgb(72,72,76));
            button(c,"W",150,topH+8,42,topH+44,tool==1?Color.rgb(66,92,135):Color.rgb(58,58,62));
            button(c,"E",198,topH+8,42,topH+44,tool==2?Color.rgb(66,92,135):Color.rgb(58,58,62));
            button(c,"R",246,topH+8,42,topH+44,tool==3?Color.rgb(66,92,135):Color.rgb(58,58,62));
            text(c,"Pivot",310,topH+31,12,Color.LTGRAY); text(c,"Local",355,topH+31,12,Color.LTGRAY);
            text(c,"2D",415,topH+31,12,Color.LTGRAY); text(c,"Gizmos",452,topH+31,12,Color.LTGRAY);
            button(c,"+",540,topH+8,42,topH+44,Color.rgb(58,72,90));
            button(c,"CAM",588,topH+8,58,topH+44,Color.rgb(58,58,62));
            button(c,"LGT",652,topH+8,58,topH+44,Color.rgb(58,58,62));
            button(c,"DEL",716,topH+8,58,topH+44,Color.rgb(92,55,58));
            button(c,"SAVE",780,topH+8,64,topH+44,Color.rgb(57,80,62));
            button(c,"IMPORT",850,topH+8,78,topH+44,Color.rgb(58,58,62));

            // Left hierarchy header
            text(c,"HIERARCHY",16,topH+toolbarH+28,13,Color.rgb(180,180,185));
            text(c,"+",leftW-32,topH+toolbarH+28,20,Color.WHITE);
            drawHierarchy(c, topH+toolbarH+46);

            // Right inspector
            text(c,"INSPECTOR",w-rightW+16,topH+toolbarH+28,13,Color.rgb(180,180,185));
            drawInspector(c,topH+toolbarH+50);

            // Viewport overlay controls / grid. The actual render is beneath this transparent region.
            float vx=leftW, vy=topH+toolbarH, vr=w-rightW, vb=h-bottomH;
            stroke.setColor(Color.argb(110,180,180,190)); stroke.setStrokeWidth(1);
            for(float x=vx+40;x<vr;x+=40)c.drawLine(x,vy,x,vb,stroke);
            for(float y=vy+40;y<vb;y+=40)c.drawLine(vx,y,vr,y,stroke);
            stroke.setColor(Color.argb(180,220,220,225)); stroke.setStrokeWidth(2);
            c.drawLine((vx+vr)/2,vy,(vx+vr)/2,vb,stroke);
            c.drawLine(vx,(vy+vb)/2,vr,(vy+vb)/2,stroke);
            fill(c,Color.argb(160,25,25,28),vx+12,vy+10,vx+142,vy+38);
            text(c,"Scene View",vx+22,vy+29,12,Color.WHITE);

            // Bottom Project / Console / Profiler.
            String[] tabs={"PROJECT","CONSOLE","PROFILER"};
            for(int i=0;i<tabs.length;i++){
                int col=i==bottomTab?Color.WHITE:Color.rgb(155,155,160);
                text(c,tabs[i],18+i*105,h-bottomH+26,12,col);
            }
            if(bottomTab==0) drawProject(c,h-bottomH+44);
            else if(bottomTab==1) drawConsole(c,h-bottomH+44);
            else drawProfiler(c,h-bottomH+44);

            // Separator lines
            stroke.setColor(Color.rgb(70,70,74)); stroke.setStrokeWidth(1);
            c.drawLine(0,topH,w,topH,stroke); c.drawLine(0,topH+toolbarH,w,topH+toolbarH,stroke);
            c.drawLine(leftW,topH+toolbarH,leftW,h-bottomH,stroke); c.drawLine(w-rightW,topH+toolbarH,w-rightW,h-bottomH,stroke); c.drawLine(0,h-bottomH,w,h-bottomH,stroke);
        }

        private void button(Canvas c,String s,float x,float y,float bw,float bh,int color){
            fill(c,color,x,y,x+bw,y+bh); stroke.setColor(Color.rgb(85,85,90)); stroke.setStrokeWidth(1); c.drawRect(x,y,x+bw,y+bh,stroke); text(c,s,x+bw/2-p.measureText(s)/2,y+bh/2+5,14,Color.WHITE);
        }

        private void drawHierarchy(Canvas c,float y){
            String[] lines=hierarchy.split("\\n");
            int row=0;
            for(String line:lines){
                if(line.trim().isEmpty())continue;
                int idx=-1; boolean sel=line.contains("*");
                String clean=line.replace("*","").trim();
                String[] parts=clean.split("\\s+",2);
                try{idx=Integer.parseInt(parts[0]);}catch(Exception ignored){}
                float ry=y+row*29;
                if(sel)fill(c,Color.rgb(52,76,110),8,ry-20,leftW-8,ry+8);
                text(c,"◆",18,ry,11,Color.rgb(110,170,230));
                String label=parts.length>1?parts[1]:"GameObject";
                text(c,label+"  "+Math.max(0,idx),38,ry,13,Color.WHITE);
                row++;
            }
            if(row==0) text(c,"Scene is empty",18,y,13,Color.GRAY);
        }

        private void drawInspector(Canvas c,float y){
            int w = getWidth();
            int h = getHeight();
            if(selectedIndex<0){ text(c,"No GameObject selected",w-rightW+16,y+16,13,Color.GRAY); return; }
            String[] ls=inspector.split("\\n");
            float yy=y;
            for(String s:ls){
                if(s.endsWith(":")){ fill(c,Color.rgb(48,48,52),w-rightW+10,yy-17,w-10,yy+10); text(c,s,w-rightW+20,yy+1,13,Color.WHITE); }
                else mono(c,s,w-rightW+20,yy,11,Color.rgb(195,195,200));
                yy+=22;
                if(yy>getHeight()-bottomH-15)break;
            }
            // Component affordance
            fill(c,Color.rgb(48,48,52),w-rightW+12,Math.min(yy+8,getHeight()-bottomH-35),w-12,Math.min(yy+40,getHeight()-bottomH-3));
            text(c,"+  Add Component",w-rightW+24,Math.min(yy+30,getHeight()-bottomH-13),12,Color.WHITE);
        }

        private void drawProject(Canvas c,float y){
            text(c,"Assets",18,y+18,13,Color.WHITE); text(c,"Scenes",110,y+18,13,Color.WHITE); text(c,"Scripts",205,y+18,13,Color.WHITE); text(c,"Modules",300,y+18,13,Color.WHITE);
            text(c,"▣  main.scene",18,y+48,12,Color.LTGRAY); text(c,"▤  project.dream",18,y+72,12,Color.GRAY); text(c,"▣  modules/",300,y+48,12,Color.LTGRAY);
        }
        private void drawConsole(Canvas c,float y){
            String[] lines=logText.split("\\n"); int start=Math.max(0,lines.length-4); float yy=y+18;
            for(int i=start;i<lines.length;i++){mono(c,lines[i],18,yy,11,Color.rgb(190,190,195));yy+=21;}
        }
        private void drawProfiler(Canvas c,float y){
            text(c,"CPU",18,y+18,12,Color.GRAY); text(c,"60 FPS target",70,y+18,12,Color.WHITE); text(c,"Editor / Runtime",190,y+18,12,Color.LTGRAY);
        }

        @Override public boolean onTouchEvent(android.view.MotionEvent e){
            float x=e.getX(), y=e.getY();
            if(e.getAction()==MotionEvent.ACTION_DOWN){
                downX=x;downY=y;draggingViewport=hit(x,y,leftW,topH+toolbarH,getWidth()-rightW,getHeight()-bottomH);return true;
            }
            if(e.getAction()==MotionEvent.ACTION_MOVE && draggingViewport){
                if(tool==1){ nativeEditorMove((x-downX)*0.01f,(downY-y)*0.01f,0); downX=x;downY=y; refreshData(); }
                return true;
            }
            if(e.getAction()!=MotionEvent.ACTION_UP)return true;

            if(hit(x,y,18,topH+8,70,topH+44)){nativeEditorSetPlaying(!nativeEditorIsPlaying());invalidate();return true;}
            if(hit(x,y,76,topH+8,128,topH+44)){nativeEditorSetPlaying(false);invalidate();return true;}
            if(hit(x,y,145,topH+8,192,topH+44)){tool=1;invalidate();return true;}
            if(hit(x,y,194,topH+8,242,topH+44)){tool=2;invalidate();return true;}
            if(hit(x,y,244,topH+8,292,topH+44)){tool=3;invalidate();return true;}
            if(hit(x,y,540,topH+8,582,topH+44)){nativeEditorCreate("entity");refreshData();return true;}
            if(hit(x,y,588,topH+8,646,topH+44)){nativeEditorCreate("camera");refreshData();return true;}
            if(hit(x,y,652,topH+8,710,topH+44)){nativeEditorCreate("light");refreshData();return true;}
            if(hit(x,y,716,topH+8,774,topH+44)){nativeEditorDelete();refreshData();return true;}
            if(hit(x,y,780,topH+8,844,topH+44)){saveScene();refreshData();return true;}
            if(hit(x,y,850,topH+8,928,topH+44)){importModules();return true;}
            if(hit(x,y,0,topH+toolbarH,leftW,getHeight()-bottomH)){
                int row=(int)((y-(topH+toolbarH+26))/29); if(row>=0){String[] ls=hierarchy.split("\\n");if(row<ls.length){try{String clean=ls[row].replace("*","").trim();int sp=clean.indexOf(' ');int idx=Integer.parseInt(sp<0?clean:clean.substring(0,sp));if(nativeEditorSelect(idx))refreshData();}catch(Exception ignored){}}} return true;
            }
            if(hit(x,y,0,getHeight()-bottomH,getWidth(),getHeight())){
                if(y>getHeight()-bottomH+5 && y<getHeight()-bottomH+45){if(x<105)bottomTab=0;else if(x<210)bottomTab=1;else bottomTab=2;invalidate();return true;}
            }
            // Double tap/click in viewport = add an entity; long press is intentionally left for future context menus.
            if(hit(x,y,leftW,topH+toolbarH,getWidth()-rightW,getHeight()-bottomH) && Math.abs(x-downX)<20 && Math.abs(y-downY)<20){
                if(tool==0 && e.getEventTime()-e.getDownTime()<250){ nativeEditorCreate("entity"); refreshData(); }
            }
            return true;
        }
    }

    // ---------- Asset extraction ----------
    private void copyBundledModules() {
        try {
            String[] names=getAssets().list("modules"); if(names==null)return;
            for(String name:names){File out=new File(moduleDir,name);if(out.exists())continue;try(InputStream in=getAssets().open("modules/"+name);OutputStream os=new FileOutputStream(out)){byte[]b=new byte[8192];int n;while((n=in.read(b))!=-1)os.write(b,0,n);}}
        }catch(IOException ignored){}
    }
    private static final String PY_MARKER=".extracted_v2";
    private void copyBundledPython(String abi,File destination){
        try{File marker=new File(destination,PY_MARKER);if(marker.isFile())return;deleteRecursive(destination);copyAssetTree("python/"+abi,destination);if(!new File(destination,"lib/python3.14/encodings").isDirectory())throw new IOException("stdlib missing");try(OutputStream os=new FileOutputStream(marker)){os.write('1');}}catch(IOException e){android.util.Log.e("DreamEngine","python extract failed",e);}
    }
    private static void deleteRecursive(File f){File[]kids=f.listFiles();if(kids!=null)for(File k:kids)deleteRecursive(k);f.delete();}
    private void copyAssetTree(String path,File target)throws IOException{String[]children=getAssets().list(path);if(children!=null&&children.length>0){if(!target.isDirectory()&&!target.mkdirs())throw new IOException("mkdir "+target);for(String child:children)copyAssetTree(path+"/"+child,new File(target,child));return;}File parent=target.getParentFile();if(parent!=null&&!parent.isDirectory()&&!parent.mkdirs())throw new IOException("mkdir "+parent);try(InputStream in=getAssets().open(path);OutputStream os=new FileOutputStream(target)){byte[]b=new byte[16384];int n;while((n=in.read(b))!=-1)os.write(b,0,n);}catch(FileNotFoundException e){target.mkdirs();}}

    @Override protected void onActivityResult(int requestCode,int resultCode,Intent data){super.onActivityResult(requestCode,resultCode,data);if(requestCode!=PICK_MODULE_DIR||resultCode!=RESULT_OK||data==null)return;Uri tree=data.getData();try{getContentResolver().takePersistableUriPermission(tree,Intent.FLAG_GRANT_READ_URI_PERMISSION);}catch(Exception ignored){}int copied=copyFromTree(tree);appendLog("imported "+copied+" file(s) (.py/.so)\n");if(copied>0)nativeReload();appendLog(nativeDrainLog());if(editorView!=null)editorView.invalidate();}
    private int copyFromTree(Uri tree){return walkTree(tree,android.provider.DocumentsContract.getTreeDocumentId(tree),0);}
    private int walkTree(Uri tree,String docId,int depth){if(depth>6)return 0;int copied=0;Uri children=android.provider.DocumentsContract.buildChildDocumentsUriUsingTree(tree,docId);String[]projection={android.provider.DocumentsContract.Document.COLUMN_DOCUMENT_ID,android.provider.DocumentsContract.Document.COLUMN_DISPLAY_NAME,android.provider.DocumentsContract.Document.COLUMN_MIME_TYPE};try(android.database.Cursor c=getContentResolver().query(children,projection,null,null,null)){if(c==null)return 0;while(c.moveToNext()){String id=c.getString(0),name=c.getString(1),mime=c.getString(2);if(name==null)continue;if(android.provider.DocumentsContract.Document.MIME_TYPE_DIR.equals(mime)){if(name.startsWith(".")||name.equals("Android"))continue;copied+=walkTree(tree,id,depth+1);continue;}if(!name.endsWith(".so")&&!name.endsWith(".py"))continue;Uri fileUri=android.provider.DocumentsContract.buildDocumentUriUsingTree(tree,id);File tmp=new File(moduleDir,name+".tmp"),out=new File(moduleDir,name);try(InputStream in=getContentResolver().openInputStream(fileUri);OutputStream os=new FileOutputStream(tmp)){byte[]b=new byte[8192];int n;while((n=in.read(b))!=-1)os.write(b,0,n);os.flush();if(out.exists())out.delete();if(tmp.renameTo(out))copied++;}catch(Exception e){tmp.delete();}}}catch(Exception ignored){}return copied;}
    @Override protected void onDestroy(){tickHandler.removeCallbacks(tick);nativeStop();super.onDestroy();}
}
