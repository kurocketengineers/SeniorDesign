package com.example.android.bluetoothlegatt;

import android.app.Activity;
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
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

/**
 * Created by eduardo on 4/29/16.
 */
public class PreFlightActivity extends Activity {
    private final static String TAG = PreFlightActivity.class.getSimpleName();

    public static final String EXTRAS_DEVICE_NAME = "DEVICE_NAME";
    public static final String EXTRAS_DEVICE_ADDRESS = "DEVICE_ADDRESS";

    private TextView mConnectionState;
    private TextView mDataField;
    private String mDeviceName;
    private String mDeviceAddress;
    private BluetoothLeService mBluetoothLeService;
    private boolean mConnected = false;

    private Button mButtonArm;
    private Button mButtonPost;
    private Button mButtonSet;

    private Map<UUID, BluetoothGattCharacteristic> map = new HashMap<UUID, BluetoothGattCharacteristic>();

    // Code to manage Service lifecycle.
    private final ServiceConnection mServiceConnection = new ServiceConnection() {

        @Override
        public void onServiceConnected(ComponentName componentName, IBinder service) {
            mBluetoothLeService = ((BluetoothLeService.LocalBinder) service).getService();
            if (!mBluetoothLeService.initialize()) {
                Log.e(TAG, "Unable to initialize Bluetooth");
                finish();
            }
            // Automatically connects to the device upon successful start-up initialization.
            mBluetoothLeService.connect(mDeviceAddress);
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {
            mBluetoothLeService = null;
        }
    };

    private final BroadcastReceiver mGattUpdateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (BluetoothLeService.ACTION_GATT_CONNECTED.equals(action)) {
                mConnected = true;
                updateConnectionState(R.string.connected);
                invalidateOptionsMenu();
            } else if (BluetoothLeService.ACTION_GATT_DISCONNECTED.equals(action)) {
                mConnected = false;
                updateConnectionState(R.string.disconnected);
                invalidateOptionsMenu();
                clearUI();
            } else if (BluetoothLeService.ACTION_GATT_SERVICES_DISCOVERED.equals(action)) {

                // Show all the supported services and characteristics on the user interface.
                //displayGattServices(mBluetoothLeService.getSupportedGattServices());
                getGattService(mBluetoothLeService.getSupportedGattService());
            } else if (BluetoothLeService.ACTION_DATA_AVAILABLE.equals(action)) {

            }
        }
    };

    private void clearUI() {

    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.pre_flight);

        //Receives Device name and adress
        final Intent intent = getIntent();
        mDeviceName = intent.getStringExtra(EXTRAS_DEVICE_NAME);
        mDeviceAddress = intent.getStringExtra(EXTRAS_DEVICE_ADDRESS);

        //Sets name and address to display
        ((TextView) findViewById(R.id.device_address)).setText(mDeviceAddress);
        mConnectionState = (TextView) findViewById(R.id.connection_state);
        mDataField = (TextView) findViewById(R.id.data_value);

        //ARM BUTTON
        mButtonArm = (Button) findViewById(R.id.button_arm);
        mButtonArm.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                arm(view);
            }
        });

        //SETTINGS BUTTON
        mButtonSet = (Button) findViewById(R.id.button_set);
        mButtonSet.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                Log.w(TAG, "Sending SET command");
                BluetoothGattCharacteristic txChar = map.get(BluetoothLeService.UUID_BLE_TX);

                byte b = 0x00;
                byte[] temp = "setthesecond".getBytes();
                byte[] tx = new byte[temp.length + 1];
                tx[0] = b;

                for (int i = 1; i < temp.length + 1; i++) {

                    tx[i] = temp[i - 1];
                }
                txChar.setValue(tx);
                mBluetoothLeService.writeCharacteristic(txChar);

                LinearLayout settings = (LinearLayout)findViewById(R.id.settings);
                settings.setVisibility(View.VISIBLE);
            }
        });

        //POST FLIGHT BUTTON
        mButtonPost = (Button)findViewById(R.id.post_flight);
        mButtonPost.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                post_flight(v);
            }
        });

        getActionBar().setTitle(mDeviceName);
        getActionBar().setDisplayHomeAsUpEnabled(true);

        Intent gattServiceIntent = new Intent(this, BluetoothLeService.class);
        bindService(gattServiceIntent, mServiceConnection, BIND_AUTO_CREATE);
    }

    @Override
    protected void onResume() {
        super.onResume();

        registerReceiver(mGattUpdateReceiver, makeGattUpdateIntentFilter());

        if (mBluetoothLeService != null) {
            final boolean result = mBluetoothLeService.connect(mDeviceAddress);
            Log.d(TAG, "Connect request result=" + result);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();

        unregisterReceiver(mGattUpdateReceiver);
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        unbindService(mServiceConnection);
        mBluetoothLeService = null;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.gatt_services, menu);
        if (mConnected) {
            menu.findItem(R.id.menu_connect).setVisible(false);
            menu.findItem(R.id.menu_disconnect).setVisible(true);
        } else {
            menu.findItem(R.id.menu_connect).setVisible(true);
            menu.findItem(R.id.menu_disconnect).setVisible(false);
        }
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch(item.getItemId()) {
            case R.id.menu_connect:
                mBluetoothLeService.connect(mDeviceAddress);
                return true;
            case R.id.menu_disconnect:
                mBluetoothLeService.disconnect();
                return true;
            case android.R.id.home:
                onBackPressed();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }


    private void updateConnectionState(final int resourceId) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mConnectionState.setText(resourceId);
            }
        });
    }

    private void getGattService(BluetoothGattService gattService) {

        if (gattService == null) {

            Log.w(TAG, "No Gatt Service found");
            return;
        }

        BluetoothGattCharacteristic characteristic = gattService.getCharacteristic(BluetoothLeService.UUID_BLE_TX);
        map.put(characteristic.getUuid(), characteristic);

        BluetoothGattCharacteristic characteristicRx = gattService.getCharacteristic(BluetoothLeService.UUID_BLE_RX);

        if (characteristicRx == null) {

            Log.w(TAG, "characteristicRx is NOT a characteristic of gattService");
            return;
        }
        mBluetoothLeService.setCharacteristicNotification(characteristicRx, true);
        mBluetoothLeService.readCharacteristic(characteristicRx);
    }


    private static IntentFilter makeGattUpdateIntentFilter() {
        final IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(BluetoothLeService.ACTION_GATT_CONNECTED);
        intentFilter.addAction(BluetoothLeService.ACTION_GATT_DISCONNECTED);
        intentFilter.addAction(BluetoothLeService.ACTION_GATT_SERVICES_DISCOVERED);
        intentFilter.addAction(BluetoothLeService.ACTION_DATA_AVAILABLE);
        return intentFilter;
    }

    //Response to pushing ARM Button
    public void arm(View view) {
        Log.w(TAG, "Sending ARM command");
        BluetoothGattCharacteristic txChar = map.get(BluetoothLeService.UUID_BLE_TX);

        byte b = 0x00;
        byte[] temp = "arm".getBytes();
        byte[] tx = new byte[temp.length + 1];
        tx[0] = b;

        for (int i = 1; i < temp.length + 1; i++) {

            tx[i] = temp[i - 1];
        }
        txChar.setValue(tx);
        mBluetoothLeService.writeCharacteristic(txChar);
    }

    //Response to pushing send apogee button
    public void sendApogee(View view) {
        // Do something in response to send pressure button
        Log.w(TAG, "Sending Apogee");
        BluetoothGattCharacteristic txChar = map.get(BluetoothLeService.UUID_BLE_TX);
        //Gets text from field
        EditText editText = (EditText) findViewById(R.id.edit_apogee);
        String message = editText.getText().toString();
        //Sends via BLE
        byte b = 0x00;
        byte[] temp = message.getBytes();
        byte[] tx = new byte[temp.length + 1];
        tx[0] = b;

        for (int i = 1; i < temp.length + 1; i++) {

            tx[i] = temp[i - 1];
        }
        txChar.setValue(tx);
        mBluetoothLeService.writeCharacteristic(txChar);
    }

    //Response to pushing post flight button
    public void post_flight(View view) {
        mBluetoothLeService.disconnect();
        final Intent intent = new Intent(this, PostFlightActivity.class);
        intent.putExtra(PostFlightActivity.EXTRAS_DEVICE_NAME, mDeviceName);
        intent.putExtra(PostFlightActivity.EXTRAS_DEVICE_ADDRESS, mDeviceAddress);
        startActivity(intent);
    }

}
