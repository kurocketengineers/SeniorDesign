package com.example.android.bluetoothlegatt;

import android.app.Activity;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.ExpandableListView;
import android.widget.ListView;
import android.widget.SimpleExpandableListAdapter;
import android.widget.TextView;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.UUID;

/**
 * Created by eduardo on 4/15/16.
 */
public class PreFlightActivity extends Activity {
    private final static String TAG = PreFlightActivity.class.getSimpleName();

    public static final String EXTRAS_DEVICE_NAME = "DEVICE_NAME";
    public static final String EXTRAS_DEVICE_ADDRESS = "DEVICE_ADDRESS";

    private TextView mDataField;
    private String mDeviceName;
    private String mDeviceAddress;
    private BluetoothLeService mBluetoothLeService;
    private boolean mConnected = false;
    private Map<UUID, BluetoothGattCharacteristic> map = new HashMap<UUID, BluetoothGattCharacteristic>();

    private Button mButtonCheck;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.pre_flight);

        final Intent intent = getIntent();
        mDeviceName = intent.getStringExtra(EXTRAS_DEVICE_NAME);
        mDeviceAddress = intent.getStringExtra(EXTRAS_DEVICE_ADDRESS);

        mButtonCheck = (Button) findViewById(R.id.button_check);
        mButtonCheck.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {

                Log.w(TAG, "Sending CHECK command");
                BluetoothGattCharacteristic txChar = map.get(BluetoothLeService.UUID_BLE_TX);

                byte b = 0x00;
                byte[] temp = "check".getBytes();
                byte[] tx = new byte[temp.length + 1];
                tx[0] = b;

                for (int i = 1; i < temp.length + 1; i++) {

                    tx[i] = temp[i - 1];
                }

                txChar.setValue(tx);
                mBluetoothLeService.writeCharacteristic(txChar);
            }
        });



        getActionBar().setTitle(mDeviceName);
        getActionBar().setDisplayHomeAsUpEnabled(true);

        //Intent gattServiceIntent = new Intent(this, BluetoothLeService.class);
        //bindService(gattServiceIntent, mServiceConnection, BIND_AUTO_CREATE);
    }

    @Override
    protected void onResume() {
        super.onResume();

        if (mBluetoothLeService != null) {
            final boolean result = mBluetoothLeService.connect(mDeviceAddress);
            Log.d(TAG, "Connect request result=" + result);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        mBluetoothLeService = null;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {

        if (item.getItemId() == android.R.id.home) {
            onBackPressed();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    public void displayData(byte[] byteArray) {

        if (byteArray != null) {

            float value = ByteBuffer.wrap(byteArray).order(ByteOrder.BIG_ENDIAN).getFloat();
            mDataField.setText(String.format("%.2f", value));
        }
    }
}
