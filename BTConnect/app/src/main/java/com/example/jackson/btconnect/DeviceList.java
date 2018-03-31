package com.example.jackson.btconnect;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import android.view.View;
import android.widget.Button;
import android.widget.ListView;

import java.util.Set;
import java.util.ArrayList;

import android.widget.ArrayAdapter;
import android.widget.AdapterView;
import android.widget.TextView;
import android.content.Intent;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;

public class DeviceList extends AppCompatActivity {

    Button btnPaired;
    ListView deviceList;
    private BluetoothAdapter myBluetooth = null;
    public static String EXTRA_ADDRESS = "B8:27:EB:BD:D3:88";

    //import android.telephony.SmsManager;
    /*private void sendSMS(String phoneNumber, String message){
        SmsManager sms = SmsManager.getDefault();
        sms.sendTextMessage(phoneNumber, null, message, null, null);
    }*/

    private void msg(String s){
        Msg.getInstance().msg(getApplicationContext(), s);
    }

    private AdapterView.OnItemClickListener myListClickListener = new AdapterView.OnItemClickListener(){
        public void onItemClick(AdapterView av, View v, int arg2, long arg3){
            //sendSMS("1234567890", "Test from android app");

            //get MAC address
            String info = ((TextView) v).getText().toString();
            String address = info.substring(info.length() - 17);
            //intent to start next activity
            Intent i = new Intent(DeviceList.this, Control.class);
            //change activity
            i.putExtra(EXTRA_ADDRESS, address); //this will be received at Control (class) Activity

            startActivity(i);
        }
    };

    private void pairedDevicesList(){
        Set<BluetoothDevice> pairedDevices = myBluetooth.getBondedDevices();
        ArrayList list = new ArrayList();

        if(pairedDevices.size() > 0){
            for(BluetoothDevice bt : pairedDevices){
                list.add(bt.getName() + "\n" + bt.getAddress());
            }
        } else {
            msg("No paired bluetooth devices found");
        }
        final ArrayAdapter adapter = new ArrayAdapter(this, android.R.layout.simple_list_item_1, list);
        deviceList.setAdapter(adapter);
        deviceList.setOnItemClickListener(myListClickListener);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_device_list);

        btnPaired = (Button)findViewById(R.id.button);
        deviceList = (ListView)findViewById(R.id.listView);

        myBluetooth = BluetoothAdapter.getDefaultAdapter();
        if(myBluetooth == null){
            msg("Bluetooth device not available");
            finish();
        } else {
            if(myBluetooth.isEnabled()){

            } else {
                Intent turnBTon = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                startActivityForResult(turnBTon, 1);
            }
        }

        btnPaired.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v){
                pairedDevicesList();
            }
        });
        pairedDevicesList();
    }
}
