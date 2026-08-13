"""
VELA FLIGHT SOFTWARE - LIVE WEB DASHBOARD
-----------------------------------------
Reads the telemetry CSV in real-time and hosts a local web server
to visualize the spacecraft's sensor data and AI anomaly triggers.
"""

import streamlit as st
import pandas as pd
import time
import os

st.set_page_config(page_title="VELA Ground Station", layout="wide")

st.title("🛰️ VELA Mission Control - Live Telemetry")
st.markdown("Real-time telemetry ingestion and Edge AI anomaly detection dashboard.")

DATA_FILE = "telemetry_dataset.csv"

# Create placeholder containers for live updates
metrics_placeholder = st.empty()
charts_placeholder = st.empty()

def load_data():
    if not os.path.exists(DATA_FILE):
        return pd.DataFrame(columns=["timestamp", "temperature_c", "radiation_hits", "attitude_rad"])
    try:
        # Read the last 100 rows for live performance
        df = pd.read_csv(DATA_FILE)
        return df.tail(100)
    except Exception:
        return pd.DataFrame(columns=["timestamp", "temperature_c", "radiation_hits", "attitude_rad"])

# Infinite loop to auto-refresh the dashboard
while True:
    df = load_data()
    
    if not df.empty:
        latest = df.iloc[-1]
        
        # 1. Live Metric Cards
        with metrics_placeholder.container():
            col1, col2, col3 = st.columns(3)
            
            # Check for AI Anomaly (Temp > 50 is our simulated failure)
            temp_status = "🔴 ANOMALY" if float(latest['temperature_c']) > 50 else "🟢 NOMINAL"
            
            col1.metric(label=f"Core Temp ({temp_status})", value=f"{latest['temperature_c']} °C")
            col2.metric(label="Radiation Hits", value=f"{latest['radiation_hits']}")
            col3.metric(label="Attitude (Rad)", value=f"{latest['attitude_rad']}")
            
        # 2. Live Graphs
        with charts_placeholder.container():
            col_a, col_b = st.columns(2)
            
            with col_a:
                st.subheader("Thermal Walk & AI Tracking")
                st.line_chart(df.set_index('timestamp')['temperature_c'], color="#FF4B4B")
                
            with col_b:
                st.subheader("Orbital Attitude Dynamics")
                st.line_chart(df.set_index('timestamp')['attitude_rad'], color="#00C0F2")
                
    time.sleep(1) # Refresh rate