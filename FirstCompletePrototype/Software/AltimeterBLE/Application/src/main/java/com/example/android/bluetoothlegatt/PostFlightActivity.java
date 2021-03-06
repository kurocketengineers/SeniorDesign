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
import android.widget.TextView;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.HashMap;
import java.util.Map;
import java.util.UUID;

/**
 * Created by eduardo on 4/29/16.
 */
public class PostFlightActivity extends Activity {
    private final static String TAG = PostFlightActivity.class.getSimpleName();

    //Device name and address
    public static final String EXTRAS_DEVICE_NAME = "DEVICE_NAME";
    public static final String EXTRAS_DEVICE_ADDRESS = "DEVICE_ADDRESS";

    // An array of floats that will hold the barometer values
    private static int ARR_SIZE = 200;
    public static float[] baroValues = new float[ARR_SIZE];
    public static int index = 0;

    private TextView mConnectionState;
    private TextView mDataField;
    private String mDeviceName;
    private String mDeviceAddress;
    private BluetoothLeService mBluetoothLeService;
    private boolean mConnected = false;

    //Buttons
    private Button mGetData;
    private Button mGraphData;

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
                getGattService(mBluetoothLeService.getSupportedGattService());
            } else if (BluetoothLeService.ACTION_DATA_AVAILABLE.equals(action)) {
                // Displays the data from the RX Characteristic
                displayData(intent.getByteArrayExtra(BluetoothLeService.EXTRA_DATA));
            }
        }
    };

    private void clearUI() {
        mDataField.setText(R.string.no_data);
    }


    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.post_flight);

        final Intent intent = getIntent();
        mDeviceName = intent.getStringExtra(EXTRAS_DEVICE_NAME);
        mDeviceAddress = intent.getStringExtra(EXTRAS_DEVICE_ADDRESS);

        ((TextView) findViewById(R.id.device_address)).setText(mDeviceAddress);
        mConnectionState = (TextView) findViewById(R.id.connection_state);
        mDataField = (TextView) findViewById(R.id.data_value);

        mGetData = (Button) findViewById(R.id.read_button);
        mGetData.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v) {
                Log.w(TAG, "Sending READ command");
                BluetoothGattCharacteristic txChar = map.get(BluetoothLeService.UUID_BLE_TX);

                index = 0;
                byte b = 0x00;
                byte[] temp = "read".getBytes();
                byte[] tx = new byte[temp.length + 1];
                tx[0] = b;

                for (int i = 1; i < temp.length + 1; i++) {

                    tx[i] = temp[i - 1];
                }

                txChar.setValue(tx);
                mBluetoothLeService.writeCharacteristic(txChar);
            }
        });

        mGraphData = (Button) findViewById(R.id.data_graph);
        mGraphData.setOnClickListener(new View.OnClickListener(){
            @Override
            public void onClick(View v) {

                graphData(v);
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



    public void displayData(byte[] byteArray) {

        float value = 0;

        if (byteArray != null) {
            //float value = ByteBuffer.wrap(byteArray).order(ByteOrder.BIG_ENDIAN).getFloat();
            value = ByteBuffer.wrap(byteArray).order(ByteOrder.BIG_ENDIAN).getFloat();
            //mDataField.setText(String.format("%.2f hPa", value));
            mDataField.setText(String.format("%.2f", value));
        }

        if (index < ARR_SIZE) {
            // Store barometer value at index
            baroValues[index] = value;
            index++;

        }
    }

    public void graphData(View view) {

        final Intent intent = new Intent(this, GraphActivity.class);
        intent.putExtra(GraphActivity.EXTRAS_DEVICE_NAME, mDeviceName);
        intent.putExtra(GraphActivity.EXTRAS_DEVICE_ADDRESS, mDeviceAddress);
       //put barometer values through to graph
        intent.putExtra("arraySize", (int)ARR_SIZE);
        Bundle b = new Bundle();
        b.putFloatArray("baroVals", baroValues);
        intent.putExtras(b);
        startActivity(intent);
    }
}
