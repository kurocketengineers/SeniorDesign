package com.example.android.bluetoothlegatt;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import com.jjoe64.graphview.GraphView;
import com.jjoe64.graphview.series.DataPoint;
import com.jjoe64.graphview.series.LineGraphSeries;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Created by eduardo on 4/15/16.
 */
public class GraphActivity extends Activity {
    private final static String TAG = GraphActivity.class.getSimpleName();

    public static final String EXTRAS_DEVICE_NAME = "DEVICE_NAME";
    public static final String EXTRAS_DEVICE_ADDRESS = "DEVICE_ADDRESS";

    private TextView mDataField;
    private String mDeviceName;
    private String mDeviceAddress;

    private BluetoothLeService mBluetoothLeService;

    private int mArraySize;
    private float[] baroValues;
    private double[] altValues;
    DataPoint[] altTemp;
    DataPoint[] baroTemp;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.data_graph);

        final Intent intent = getIntent();
        mDeviceName = intent.getStringExtra(EXTRAS_DEVICE_NAME);
        mDeviceAddress = intent.getStringExtra(EXTRAS_DEVICE_ADDRESS);

        // Get array size of barometer values
        Bundle extras = getIntent().getExtras();
        mArraySize = extras.getInt("arraySize");

        // Make array to hold barometer values
        Bundle b = getIntent().getExtras();
        baroValues = new float[mArraySize];
        baroValues = b.getFloatArray("baroVals");

        // Array to hold altitude values
        altValues = new double[mArraySize];

        // Allocate memory for graph values
        altTemp = new DataPoint[mArraySize];
        baroTemp = new DataPoint[mArraySize];

        // Convert barometer pressure values to altitude
        for (int i = 0; i < mArraySize; i++) {

            double holdDouble = 0;
            holdDouble = 44330.0 * (1.0 - Math.pow(baroValues[i]/1013.25, 0.1903));
            // Convert double value to two decimal places
            altValues[i] = Math.floor(holdDouble * 100) / 100;
        }

        GraphView altGraph = (GraphView) findViewById(R.id.alt_graph);

        for (int i = 0; i < mArraySize; i++) {

            altTemp[i] = new DataPoint((double)i, altValues[i]);
        }

        LineGraphSeries<DataPoint> altSeries = new LineGraphSeries<>(altTemp);

        altGraph.setTitle("Altitude Graph");
        altGraph.getGridLabelRenderer().setHorizontalAxisTitle("Values");
        altGraph.getGridLabelRenderer().setVerticalAxisTitle("Feet");
     //   altGraph.getViewport().setMaxX(3000.0);
     //   altGraph.getViewport().setXAxisBoundsManual(true);
        altGraph.addSeries(altSeries);

        GraphView baroGraph = (GraphView) findViewById(R.id.baro_graph);

        for (int i = 0; i < mArraySize; i++){

            baroTemp[i] = new DataPoint((double)i, (double)baroValues[i]);
        }

        LineGraphSeries<DataPoint> baroSeries = new LineGraphSeries<>(baroTemp);

        baroGraph.setTitle("Barometer Graph");
        baroGraph.getGridLabelRenderer().setHorizontalAxisTitle("Values");
        baroGraph.getGridLabelRenderer().setVerticalAxisTitle("hPa");
       // baroGraph.getViewport().setMaxX(3000.0);
      //  baroGraph.getViewport().setXAxisBoundsManual(true);
        baroGraph.addSeries(baroSeries);

        getActionBar().setTitle(mDeviceName);
        getActionBar().setDisplayHomeAsUpEnabled(true);
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

}
