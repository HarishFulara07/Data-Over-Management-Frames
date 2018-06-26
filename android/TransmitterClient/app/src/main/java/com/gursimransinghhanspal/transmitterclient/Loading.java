package com.gursimransinghhanspal.transmitterclient;


import android.app.Dialog;
import android.content.Context;
import android.os.Bundle;
import android.support.annotation.NonNull;

public class Loading extends Dialog {

    protected Loading(@NonNull Context context) {
        super(context, false, null);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.dialog_loading);
    }


}
