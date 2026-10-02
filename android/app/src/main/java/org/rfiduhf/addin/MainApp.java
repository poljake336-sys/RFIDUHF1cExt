/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Внешняя компонента RFIDUHF для мобильной платформы 1С (Android).
// Чтение UHF RFID через DeviceAPI Chainway (RFIDWithUHFUART), типично C72.
//
// Автор: Каюмов А.Р.
// Дата:  02.10.2026
//
// Каркас Native AddIn: по мотивам шаблона androidUtils1cExt (Infostart / Igor Kisil).
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

package org.rfiduhf.addin;

import android.app.Activity;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

import com.rscja.deviceapi.RFIDWithUHFUART;
import com.rscja.deviceapi.entity.UHFTAGInfo;
import com.rscja.deviceapi.interfaces.IUHFInventoryCallback;

import org.json.JSONObject;

/**
 * Java-слой компоненты.
 * 1С вызывает методы через C++/JNI; метки уходят обратно событием TagRead.
 */
public class MainApp implements Runnable {

    private static final String TAG = "RFIDUHF";

    // JNI: Inf18/MainApp.cpp → Java_org_rfiduhf_addin_MainApp_onTagRead / onError
    static native void onTagRead(long pObject, String json);

    static native void onError(long pObject, String message);

    private final long v8Object;
    private final Activity activity;
    private final Handler mainHandler = new Handler(Looper.getMainLooper());

    private RFIDWithUHFUART reader;
    private volatile boolean inventoryRunning;
    private volatile boolean connected;
    private String lastError = "";

    public MainApp(Activity activity, long v8Object) {
        this.activity = activity;
        this.v8Object = v8Object;
    }

    @Override
    public void run() {
        // .so с JNI; если AddIn уже загружен платформой 1С — UnsatisfiedLinkError нормален.
        try {
            System.loadLibrary("org_rfiduhf_addin");
        } catch (UnsatisfiedLinkError e) {
            Log.w(TAG, "loadLibrary: " + e.getMessage());
        } catch (Exception e) {
            Log.e(TAG, "loadLibrary failed", e);
        }
    }

    public void show() {
        if (activity != null) {
            activity.runOnUiThread(this);
        } else {
            run();
        }
    }

    /**
     * Открыть UHF-модуль. Без успешного init дальше читать нельзя.
     */
    public synchronized boolean initialize() {
        try {
            if (reader == null) {
                reader = RFIDWithUHFUART.getInstance();
            }
            if (reader == null) {
                setError("Не удалось получить RFIDWithUHFUART (нет UHF-модуля?)");
                connected = false;
                return false;
            }
            if (activity == null) {
                setError("Нет Activity — init(Context) невозможен");
                connected = false;
                return false;
            }

            Context ctx = activity.getApplicationContext();
            boolean ok = reader.init(ctx);
            connected = ok;
            if (!ok) {
                setError("UHF init() вернул false. Проверьте ТСД/прошивку/права UART.");
            } else {
                lastError = "";
                Log.i(TAG, "UHF initialized: " + getDeviceInfo());
            }
            return ok;
        } catch (Exception e) {
            connected = false;
            setError("initialize: " + safeMsg(e));
            return false;
        }
    }

    /**
     * Обязательно при закрытии формы 1С — иначе UART часто «залипает» до reboot.
     */
    public synchronized boolean shutdown() {
        boolean ok = true;
        try {
            stopInventory();
            ok = releaseReader();
            connected = false;
            inventoryRunning = false;
        } catch (Exception e) {
            setError("shutdown: " + safeMsg(e));
            ok = false;
        }
        return ok;
    }

    private boolean releaseReader() {
        if (reader == null) {
            return true;
        }
        try {
            boolean ok = reader.free();
            reader = null;
            return ok;
        } catch (Exception e) {
            reader = null;
            setError("free(): " + safeMsg(e));
            return false;
        }
    }

    /**
     * Непрерывная инвентаризация. Метки — события TagRead (JSON).
     * Схема как в официальном uhf-uart-demo: setInventoryCallback + startInventoryTag.
     */
    public synchronized boolean startInventory() {
        try {
            if (!ensureReader()) {
                return false;
            }
            if (inventoryRunning) {
                return true;
            }

            clearInventoryCallback();

            reader.setInventoryCallback((UHFTAGInfo info) -> {
                // DeviceAPI зовёт с рабочего потока — в 1С отдаём с главного.
                final UHFTAGInfo tag = info;
                mainHandler.post(() -> {
                    if (inventoryRunning) {
                        emitTag(tag);
                    }
                });
            });

            // Флаг ДО start: иначе первые callback'и успеют прийти и будут отброшены.
            inventoryRunning = true;
            boolean ok = reader.startInventoryTag();
            if (!ok) {
                inventoryRunning = false;
                clearInventoryCallback();
                setError("startInventoryTag() вернул false");
            } else {
                lastError = "";
                Log.i(TAG, "inventory started");
            }
            return ok;
        } catch (Exception e) {
            inventoryRunning = false;
            clearInventoryCallback();
            setError("startInventory: " + safeMsg(e));
            return false;
        }
    }

    public synchronized boolean stopInventory() {
        inventoryRunning = false;
        try {
            if (reader == null) {
                return true;
            }
            clearInventoryCallback();
            boolean ok = reader.stopInventory();
            Log.i(TAG, "inventory stopped, ok=" + ok);
            return ok;
        } catch (Exception e) {
            setError("stopInventory: " + safeMsg(e));
            return false;
        }
    }

    private void clearInventoryCallback() {
        if (reader == null) {
            return;
        }
        try {
            reader.setInventoryCallback(null);
        } catch (Exception e) {
            // На части прошивок сброс callback после free/stop кидает — для stop это некритично.
            Log.w(TAG, "setInventoryCallback(null): " + safeMsg(e));
        }
    }

    /**
     * Однократное считывание ближайшей метки (без непрерывного inventory).
     */
    public synchronized boolean inventorySingle() {
        try {
            if (!ensureReader()) {
                return false;
            }
            UHFTAGInfo info = reader.inventorySingleTag();
            if (info == null) {
                setError("Метка не считана (inventorySingleTag=null)");
                return false;
            }
            emitTag(info);
            lastError = "";
            return true;
        } catch (Exception e) {
            setError("inventorySingle: " + safeMsg(e));
            return false;
        }
    }

    /**
     * Мощность 5..30 dBm. Меньше — уже зона («только эта полка»).
     */
    public synchronized boolean setPower(int dbm) {
        try {
            if (!ensureReader()) {
                return false;
            }
            int power = dbm;
            if (power < 5) {
                power = 5;
            }
            if (power > 30) {
                power = 30;
            }
            boolean ok = reader.setPower(power);
            if (!ok) {
                setError("setPower(" + power + ") вернул false");
            } else {
                lastError = "";
            }
            return ok;
        } catch (Exception e) {
            setError("setPower: " + safeMsg(e));
            return false;
        }
    }

    public synchronized int getPower() {
        try {
            if (reader == null) {
                return -1;
            }
            return reader.getPower();
        } catch (Exception e) {
            setError("getPower: " + safeMsg(e));
            return -1;
        }
    }

    /**
     * Регион частоты (значения — из demo/документации Chainway под вашу прошивку).
     */
    public synchronized boolean setFrequencyMode(int mode) {
        try {
            if (!ensureReader()) {
                return false;
            }
            boolean ok = reader.setFrequencyMode(mode);
            if (!ok) {
                setError("setFrequencyMode(" + mode + ") вернул false");
            } else {
                lastError = "";
            }
            return ok;
        } catch (Exception e) {
            setError("setFrequencyMode: " + safeMsg(e));
            return false;
        }
    }

    public boolean isConnected() {
        return connected;
    }

    public boolean isInventoryRunning() {
        return inventoryRunning;
    }

    public String getLastError() {
        return lastError != null ? lastError : "";
    }

    public String getDeviceInfo() {
        try {
            String base = android.os.Build.MANUFACTURER + ":" + android.os.Build.MODEL;
            if (reader == null) {
                return base;
            }
            return base
                    + ";UHF_SW=" + nullToEmpty(reader.getVersion())
                    + ";UHF_HW=" + nullToEmpty(reader.getHardwareVersion());
        } catch (Exception e) {
            return android.os.Build.MANUFACTURER + ":" + android.os.Build.MODEL;
        }
    }

    /**
     * Курок пистолета C72. В МП 1С надёжнее кнопки формы; это для debug Activity.
     */
    public boolean handleTriggerKey(int keyCode, boolean keyDown) {
        if (!isTriggerKey(keyCode)) {
            return false;
        }
        if (keyDown) {
            startInventory();
        } else {
            stopInventory();
        }
        return true;
    }

    public static boolean isTriggerKey(int keyCode) {
        switch (keyCode) {
            case 139:
            case 280:
            case 291:
            case 293: // KEYCODE_F9 на многих прошивках Chainway
            case 294:
            case 311:
            case 312:
            case 313:
            case 315:
            case 591:
            case 593:
            case 594:
            case 596:
                return true;
            default:
                return false;
        }
    }

    private boolean ensureReader() {
        if (reader != null && connected) {
            return true;
        }
        return initialize();
    }

    private void emitTag(UHFTAGInfo info) {
        if (info == null || v8Object == 0) {
            return;
        }
        String epc = info.getEPC();
        if (epc == null || epc.isEmpty() || epc.matches("0+")) {
            return;
        }
        try {
            JSONObject json = new JSONObject();
            json.put("epc", epc);
            json.put("tid", nullToEmpty(info.getTid()));
            json.put("rssi", nullToEmpty(info.getRssi()));
            json.put("ts", System.currentTimeMillis());
            onTagRead(v8Object, json.toString());
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "onTagRead native missing", e);
        } catch (Exception e) {
            Log.e(TAG, "emitTag failed", e);
        }
    }

    private void setError(String message) {
        lastError = message != null ? message : "";
        Log.e(TAG, lastError);
        if (v8Object == 0) {
            return;
        }
        final String msg = lastError;
        mainHandler.post(() -> {
            try {
                onError(v8Object, msg);
            } catch (UnsatisfiedLinkError e) {
                Log.e(TAG, "onError native missing", e);
            } catch (Exception e) {
                // Не роняем UI из‑за сбоя доставки ошибки в 1С.
                Log.w(TAG, "onError dispatch failed: " + safeMsg(e));
            }
        });
    }

    private static String nullToEmpty(String s) {
        return s != null ? s : "";
    }

    private static String safeMsg(Exception e) {
        if (e == null) {
            return "unknown";
        }
        String m = e.getMessage();
        return m != null ? m : e.getClass().getSimpleName();
    }
}
