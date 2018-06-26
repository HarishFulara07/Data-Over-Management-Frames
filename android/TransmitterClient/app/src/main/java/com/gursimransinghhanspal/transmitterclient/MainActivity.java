package com.gursimransinghhanspal.transmitterclient;

import android.content.Context;
import android.content.DialogInterface;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.SwitchCompat;
import android.text.TextUtils;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import java.net.NetworkInterface;
import java.net.SocketException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Enumeration;
import java.util.Locale;

public class MainActivity extends AppCompatActivity {

	private static final String TAG = MainActivity.class.getSimpleName();
	private static final int ONE_TIME_BROADCAST_DELIVERY = 1;
	private static final int GUARANTEED_DELIVERY_EFFORT = 2;

	SwitchCompat mGuaranteedDeliverySwitch;
	SwitchCompat mOneTimeBroadcastSwitch;
	private EditText mIeDataEditText;
	private TextView mOutputLog;

	/* ***
	 * Load the native libraries on application startup
	 */
	static {
		System.loadLibrary("jni-handler");
	}

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);

		mGuaranteedDeliverySwitch = findViewById(R.id.resLayout_activityMain_guaranteedDelivery_switch);
		mOneTimeBroadcastSwitch = findViewById(R.id.resLayout_activityMain_oneTimeBroadcast_switch);
		mIeDataEditText = findViewById(R.id.resLayout_activityMain_ieDataInput);
		mOutputLog = findViewById(R.id.resLayout_activityMain_outputLog);


		// default values
		mGuaranteedDeliverySwitch.setChecked(true);
		mOneTimeBroadcastSwitch.setChecked(false);

		// click listeners
		mGuaranteedDeliverySwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
				mOneTimeBroadcastSwitch.setChecked(!b);
			}
		});
		mOneTimeBroadcastSwitch.setOnCheckedChangeListener(new CompoundButton.OnCheckedChangeListener() {
			@Override
			public void onCheckedChanged(CompoundButton compoundButton, boolean b) {
				mGuaranteedDeliverySwitch.setChecked(!b);
			}
		});
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
		final UserInput userInput = parseInput();
		if (userInput == null) {
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
			onInterfaceSelected(networkInterfaces.get(0), userInput.data, userInput.effort);
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
						onInterfaceSelected(networkInterfaces.get(i), userInput.data, userInput.effort);
					}
				}
		);
		builder.create().show();
	}

	/* *
	 * Helper Methods
	 */

	private UserInput parseInput() {

		int inputEffort;
		if (mGuaranteedDeliverySwitch.isChecked()) {
			inputEffort = GUARANTEED_DELIVERY_EFFORT;
		} else if (mOneTimeBroadcastSwitch.isChecked()) {
			inputEffort = ONE_TIME_BROADCAST_DELIVERY;
		} else {
			Toast.makeText(
					MainActivity.this,
					"Invalid effort value!",
					Toast.LENGTH_SHORT
			).show();
			return null;
		}

		String inputData = mIeDataEditText.getText().toString().trim();
		if (TextUtils.isEmpty(inputData)) {
			Toast.makeText(
					MainActivity.this,
					"Data can't be empty!",
					Toast.LENGTH_SHORT
			).show();
			return null;
		}

		UserInput userInput = new UserInput();
		userInput.effort = inputEffort;
		userInput.data = inputData;
		return userInput;
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

	private class UserInput {
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

	/**
	 * Native Methods
	 */

	// default method
	// good to check that ndk integration is actually working!
	public native String stringFromJNI();

	// our main method
	native void setInformationElementsInProbeRequests(String interfaceName, String ieData, int effort);
}
