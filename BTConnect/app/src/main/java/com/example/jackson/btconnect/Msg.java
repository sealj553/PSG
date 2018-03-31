package com.example.jackson.btconnect;

import android.content.Context;
import android.os.Handler;
import android.widget.Toast;

class Msg {
    private static final Msg ourInstance = new Msg();

    static Msg getInstance() {
        return ourInstance;
    }

    private Msg() {
    }

    Handler messageHandler = new Handler();
    Toast lastToast = null;

    public void msg(final Context context, final String text) {
        Runnable displayToast = new Runnable() {
            public void run() {
                if(lastToast != null) {
                    lastToast.cancel();
                }
                lastToast = Toast.makeText(context, text, Toast.LENGTH_SHORT);
                lastToast.show();
            }
        };
        messageHandler.post(displayToast);
    }
}
