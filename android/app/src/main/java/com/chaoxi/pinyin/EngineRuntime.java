package com.chaoxi.pinyin;
import android.content.Context;
import android.content.res.AssetManager;
import java.io.*;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
final class EngineRuntime {
    static final ExecutorService worker = Executors.newSingleThreadExecutor();
    private static boolean initialized;
    static void ensure(Context context) throws IOException {
        if (initialized) return;
        File runtime = new File(context.getFilesDir(), "runtime-0.2.0");
        File marker = new File(runtime, "ready");
        if (!marker.isFile()) {
            copy(context.getAssets(), "data", new File(runtime, "data"));
            try (FileOutputStream out = new FileOutputStream(marker)) { out.write(1); out.getFD().sync(); }
        }
        File home = new File(context.getFilesDir(), "UserData");
        if (!home.isDirectory() && !home.mkdirs()) throw new IOException("Cannot create data directory");
        EngineBridge.initialize(runtime.getAbsolutePath(), home.getAbsolutePath());
        initialized = true;
    }
    private static void copy(AssetManager assets, String source, File target) throws IOException {
        String[] children = assets.list(source);
        if (children != null && children.length > 0) {
            if (!target.isDirectory() && !target.mkdirs()) throw new IOException("Cannot create runtime directory");
            for (String child : children) copy(assets, source + "/" + child, new File(target, child));
        } else {
            try (InputStream input = assets.open(source); FileOutputStream output = new FileOutputStream(target)) {
                byte[] buffer = new byte[32768]; int n;
                while ((n = input.read(buffer)) != -1) output.write(buffer, 0, n);
                output.getFD().sync();
            }
        }
    }
    private EngineRuntime() {}
}
