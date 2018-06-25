package com.gursimransinghhanspal.transmitterclient;

import android.content.Context;
import android.content.DialogInterface;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.TextUtils;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;

import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Enumeration;
import java.util.Locale;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("jni-handler");
    }

    private EditText mIeCountEditText;
    private EditText mIeDataEditText;
    private TextView mOutputLog;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        ImageButton mIeCountIncreaseButton = findViewById(R.id.resLayout_activityMain_ieCountSpinner_incButton);
        ImageButton mIeCountDecreaseButton = findViewById(R.id.resLayout_activityMain_ieCountSpinner_decButton);
        mIeCountEditText = findViewById(R.id.resLayout_activityMain_ieCountInput);
        mIeDataEditText = findViewById(R.id.resLayout_activityMain_ieDataInput);
        mOutputLog = findViewById(R.id.resLayout_activityMain_outputLog);

        mIeCountIncreaseButton.setOnClickListener(this);
        mIeCountDecreaseButton.setOnClickListener(this);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.activity_main_options, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case R.id.resMenu_activityMain_transmit:
                onTransmit();
                return true;
        }
        return false;
    }


    @Override
    public void onClick(View view) {

        switch (view.getId()) {
            case R.id.resLayout_activityMain_ieCountSpinner_incButton:
                onIncreaseCount();
                break;

            case R.id.resLayout_activityMain_ieCountSpinner_decButton:
                onDecreaseCount();
                break;
        }
    }

    /* *
     * onClick Handlers.
     */

    private void onIncreaseCount() {
        String countAsString = mIeCountEditText.getText().toString().trim();
        Integer count;
        try {
            count = Integer.parseInt(countAsString);
        } catch (NumberFormatException nfe) {
            Toast.makeText(
                    MainActivity.this,
                    "Invalid IE count!",
                    Toast.LENGTH_SHORT
            ).show();
            return;
        }

        if (count < 1) {
            count = 1;
        } else {
            count += 1;
        }

        if (count > 2) {
            count = 2;
        }

        mIeCountEditText.setText(String.format(Locale.US, "%d", count));
    }

    private void onDecreaseCount() {
        String countAsString = mIeCountEditText.getText().toString().trim();
        Integer count;
        try {
            count = Integer.parseInt(countAsString);
        } catch (NumberFormatException nfe) {
            Toast.makeText(
                    MainActivity.this,
                    "Invalid IE count!",
                    Toast.LENGTH_SHORT
            ).show();
            return;
        }

        if (count <= 1) {
            count = 1;
        } else {
            count -= 1;
        }

        mIeCountEditText.setText(String.format(Locale.US, "%d", count));
    }

    /**
     * Called when transmit button is clicked
     * - Parses the Input data
     * - Enabled WIFI if turned off
     * - Opens a dialog selector to
     */
    private void onTransmit() {

        clearLogs();

        /*
         * Parse the input
         */
        final InputData ieData = parseInput();
        if (ieData == null) {
            return;
        }

        /* *
         * Enable WiFi
         */
        WifiManager manager = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        if (manager == null) {
            appendToLogs(LogTag.ERR, "Could not acquire Context.WIFI_SERVICE!");
            return;
        }

        if (!manager.isWifiEnabled()) {
            appendToLogs(LogTag.INF, "Device WIFI is not enabled. Enabling...");
            manager.setWifiEnabled(true);
        } else {
            appendToLogs(LogTag.INF, "Device WIFI is enabled.");
        }
        appendToLogs(LogTag.EMPTY, null);

        /* *
         * Find wireless interface name
         */
        appendToLogs(LogTag.INF, "Detecting all available interfaces...");
        final ArrayList<NetworkInterface> networkInterfaces = new ArrayList<>();
        try {
            for (Enumeration<NetworkInterface> list = NetworkInterface.getNetworkInterfaces(); list.hasMoreElements(); ) {
                networkInterfaces.add(list.nextElement());
            }
        } catch (SocketException | NullPointerException ignored) {

        }

        // corner cases!
        if (networkInterfaces.size() == 0) {
            appendToLogs(LogTag.ERR, "No interfaces found!");
            return;
        }
        if (networkInterfaces.size() == 1) {
            appendToLogs(LogTag.INF, "Only single interface available. Proceeding...");
            onInterfaceSelected(networkInterfaces.get(0), ieData.data, ieData.effort);
            return;
        }

        // sort for visual ease
        Collections.sort(networkInterfaces, new Comparator<NetworkInterface>() {
            @Override
            public int compare(NetworkInterface networkInterface, NetworkInterface t1) {
                return networkInterface.getIndex() - t1.getIndex();
            }
        });

        // show list to choose
        String[] interfaceNames = new String[networkInterfaces.size()];
        for (int i = 0; i < networkInterfaces.size(); i++) {
            interfaceNames[i] = String.format(Locale.US, "%d. %s", networkInterfaces.get(i).getIndex(), networkInterfaces.get(i).getDisplayName());
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Select Network Interface");
        builder.setItems(
                interfaceNames,
                new DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialogInterface, int i) {
                        onInterfaceSelected(networkInterfaces.get(i), ieData.data, ieData.effort);
                    }
                }
        );
        builder.create().show();
    }

    /* *
     * Helper Methods
     */

    private InputData parseInput() {
        String countAsString = mIeCountEditText.getText().toString().trim();
        Integer count;
        try {
            count = Integer.parseInt(countAsString);
        } catch (NumberFormatException nfe) {
            Toast.makeText(
                    MainActivity.this,
                    "Invalid effort!",
                    Toast.LENGTH_SHORT
            ).show();
            return null;
        }

        if (count != 1 && count != 2) {
            Toast.makeText(
                    MainActivity.this,
                    "Effort can only be 1 or 2!",
                    Toast.LENGTH_SHORT
            ).show();
            return null;
        }

        String dataAsSingleString = mIeDataEditText.getText().toString().trim();
        if (TextUtils.isEmpty(dataAsSingleString)) {
            Toast.makeText(
                    MainActivity.this,
                    "Data can't be empty!",
                    Toast.LENGTH_SHORT
            ).show();
            return null;
        }

        InputData inputData = new InputData();
        inputData.effort = count;
        inputData.data = dataAsSingleString;
        return inputData;
    }

    /**
     * Called when the user selects an Interface from the list
     * <p>
     * - Calls the native code to stuff Information Elements
     * - Initiates a WIFI scan after the stuffing
     */
    private void onInterfaceSelected(NetworkInterface networkInterface, String ieData, int effort) {

        appendToLogs(LogTag.INF, String.format("<%s> interface selected.", networkInterface.getDisplayName()));
        appendToLogs(LogTag.INF, "Attempting to stuff Information Elements...");

        // call native probe stuffing code here!
        setInformationElementsInProbeRequests(
                networkInterface.getName(),
                ieData,
                effort
        );
        appendToLogs(LogTag.INF, "Stuffing done!");

        // initiate scan
        WifiManager manager = (WifiManager) getApplicationContext().getSystemService(Context.WIFI_SERVICE);
        if (manager == null) {
            appendToLogs(LogTag.ERR, "Could not acquire Context.WIFI_SERVICE!");
            return;
        }
        appendToLogs(LogTag.INF, "Initiating WIFI scan.");
        manager.startScan();
    }

    private void clearLogs() {
        mOutputLog.setText(null);
    }

    private void appendToLogs(LogTag tag, String string) {
        String logs = mOutputLog.getText().toString();

        switch (tag) {
            case ERR:
                logs += "ERR: " + string;
                break;

            case INF:
                logs += "INF: " + string;
                break;

            case EMPTY:
                break;
        }

        logs += "\n";
        mOutputLog.setText(logs);
    }

    private class InputData {
        int effort;
        String data;
    }

    /* *
     * Private Helper Enum
     */
    private enum LogTag {
        ERR,
        INF,
        EMPTY
    }

    /* *
     * Native Methods
     */
    static {
        System.loadLibrary("jni-handler");
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();

    native void setInformationElementsInProbeRequests(String interfaceName, String ieData, int effort);
}
