/////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Отладочный Activity (вне 1С). В мобильном клиенте 1С не используется.
//
// Автор: Каюмов А.Р.
// Дата:  02.10.2026
//
/////////////////////////////////////////////////////////////////////////////////////////////////////

package org.rfiduhf.addin;

import android.app.Activity;
import android.os.Bundle;
import android.view.KeyEvent;
import android.widget.TextView;

public class MainActivity extends Activity {

    private MainApp bridge;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        TextView tv = new TextView(this);
        tv.setText("RFIDUHF AddIn\nОтладочный хост.\nРабочая точка входа — мобильная платформа 1С.");
        tv.setPadding(48, 48, 48, 48);
        setContentView(tv);

        // v8Object=0: события TagRead в native не уйдут (нет контекста 1С).
        bridge = new MainApp(this, 0);
        bridge.show();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (bridge != null && MainApp.isTriggerKey(keyCode) && event.getRepeatCount() == 0) {
            bridge.handleTriggerKey(keyCode, true);
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        if (bridge != null && MainApp.isTriggerKey(keyCode)) {
            bridge.handleTriggerKey(keyCode, false);
            return true;
        }
        return super.onKeyUp(keyCode, event);
    }

    @Override
    protected void onDestroy() {
        if (bridge != null) {
            bridge.shutdown();
            bridge = null;
        }
        super.onDestroy();
    }
}
