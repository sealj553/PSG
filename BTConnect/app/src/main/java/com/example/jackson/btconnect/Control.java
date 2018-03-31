package com.example.jackson.btconnect;

import android.media.MediaPlayer;
import android.os.Handler;
import android.speech.tts.TextToSpeech;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import android.bluetooth.BluetoothSocket;
import android.text.method.ScrollingMovementMethod;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.app.ProgressDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.os.AsyncTask;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.util.Locale;

public class Control extends AppCompatActivity {

    Button btnDis, btnSend;
    EditText textInput;
    TextView recvTextView;
    String address = null;
    private ProgressDialog progress;
    BluetoothAdapter myBluetooth;
    BluetoothSocket btSocket;
    private boolean isBtConnected = false;

    boolean stopWorker;
    int readBufferPosition;
    byte[] readBuffer;
    Thread workerThread;

    long lastMessageTime;

    TextToSpeech tts;

    private String processText(String txt){
        if(txt.equals("s")){
            msg("CALLING 911");
            txt = "The authorities have been alerted";

            String utteranceId = this.hashCode() + "";
            tts.speak("the police are on their way", TextToSpeech.QUEUE_FLUSH, null, utteranceId);
        }

        return txt;
    }

    private void msg(String s){
        Msg.getInstance().msg(getApplicationContext(), s);
    }

    private void disconnect(){
        if(btSocket != null){ //btSocket is busy
            try{
                btSocket.close();
                stopWorker = true;
            } catch (IOException e){
                msg("Error");
            }
        }
        finish();
    }

    private void sendText(String txt){
        if(btSocket != null){
            try{
                btSocket.getOutputStream().write(txt.getBytes());
                btSocket.getOutputStream().flush();
            } catch (IOException e){
                msg("Error");
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_led_control);

        //get address of device
        address = getIntent().getStringExtra(DeviceList.EXTRA_ADDRESS);

        //view of Control layout
        setContentView(R.layout.activity_led_control);

        //widgets
        btnDis = (Button)findViewById(R.id.button4);
        btnSend = (Button)findViewById(R.id.sendButton);
        textInput = (EditText)findViewById(R.id.editText);
        recvTextView = (TextView)findViewById(R.id.recvTextView);
        recvTextView.setMovementMethod(new ScrollingMovementMethod());

        btnDis.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v){
                disconnect();
                msg("Disconnected");
            }
        });
        btnSend.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v){
                sendText(textInput.getText().toString());
                textInput.setText("");
            }
        });

        textInput.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                boolean handled = false;
                if (actionId == EditorInfo.IME_ACTION_SEND) {
                    sendText(textInput.getText().toString());
                    textInput.setText("");
                    handled = true;
                }
                return handled;
            }
        });

        tts = new TextToSpeech(getApplicationContext(), new TextToSpeech.OnInitListener() {
            @Override
            public void onInit(int status) {
                if(status != TextToSpeech.ERROR) {
                    tts.setLanguage(Locale.UK);
                }
            }
        });

        new ConnectBT().execute();
    }

    void beginListenForData(){

        final Handler handler = new Handler();
        final byte delimiter = '\n';

        stopWorker = false;
        readBufferPosition = 0;
        readBuffer = new byte[64];
        workerThread = new Thread(new Runnable(){
            public void run(){
                lastMessageTime = System.currentTimeMillis();

                while(!Thread.currentThread().isInterrupted() && !stopWorker){
                    if(System.currentTimeMillis() - lastMessageTime > 3000){
                        disconnect();
                        msg("Connection timed out");
                    }

                    try {
                        int bytesAvailable = btSocket.getInputStream().available();
                        if(bytesAvailable > 0){
                            byte[] packetBytes = new byte[bytesAvailable];
                            btSocket.getInputStream().read(packetBytes);
                            for(int i = 0; i < bytesAvailable; i++){
                                byte b = packetBytes[i];
                                if(b == delimiter){
                                    byte[] encodedBytes = new byte[readBufferPosition];
                                    System.arraycopy(readBuffer, 0, encodedBytes, 0, encodedBytes.length);
                                    final String data = new String(encodedBytes, "US-ASCII");
                                    readBufferPosition = 0;

                                    handler.post(new Runnable(){
                                        public void run(){
                                            lastMessageTime = System.currentTimeMillis();
                                            String text = processText(data);
                                            if(text.equals("")) {
                                                recvTextView.append(text);
                                            } else {
                                                recvTextView.append(text + "\n");
                                            }
                                            final int scrollAmount = recvTextView.getLayout().getLineTop(recvTextView.getLineCount()) - recvTextView.getHeight();
                                            if (scrollAmount > 0) {
                                                recvTextView.scrollTo(0, scrollAmount);
                                            } else {
                                                recvTextView.scrollTo(0, 0);
                                            }
                                        }
                                    });
                                } else {
                                    readBuffer[readBufferPosition++] = b;
                                }
                            }
                        }
                    } catch (IOException ex){
                        stopWorker = true;
                    }
                }
            }
        });
        workerThread.start();
    }

    //UI thread
    private class ConnectBT extends AsyncTask<Void, Void, Void> {
        private boolean ConnectSuccess = true;

        @Override
        protected void onPreExecute(){
            progress = ProgressDialog.show(Control.this, "Connecting...", "Please wait");
        }

        //connect in background while progress dialog is shown
        @Override
        protected Void doInBackground(Void... devices) {
            try{
                if(btSocket == null || !isBtConnected){
                    myBluetooth = BluetoothAdapter.getDefaultAdapter(); //connect to device's address and check if it's available

                    BluetoothDevice device = myBluetooth.getRemoteDevice(address);
                    //btSocket = device.createInsecureRfcommSocketToServiceRecord(myUUID);
                    btSocket = (BluetoothSocket)device.getClass().getMethod("createRfcommSocket", new Class[] {int.class}).invoke(device, 1);
                    BluetoothAdapter.getDefaultAdapter().cancelDiscovery();
                    btSocket.connect();
                }
            } catch (IOException | NoSuchMethodException | IllegalAccessException | InvocationTargetException e){
                ConnectSuccess = false;
            }
            return null;
        }

        @Override
        protected void onPostExecute(Void result){ //checks status after doInBackground
            super.onPostExecute(result);
            if(!ConnectSuccess){
                msg("Connection Failed. Try again.");
                finish();
            } else {
                msg("Connected");
                beginListenForData();
                isBtConnected = true;
            }
            progress.dismiss();
        }
    }
}
