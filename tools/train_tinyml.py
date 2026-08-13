#!/usr/bin/env python3
"""
VELA FLIGHT SOFTWARE - TINYML MODEL GENERATOR
---------------------------------------------
Reads telemetry_dataset.csv, trains a statistical anomaly detection 
model (Z-Score / Envelope), and exports the weights as a C header 
for bare-metal STM32 inference.
"""

import os
import sys
import csv

CSV_FILE = "telemetry_dataset.csv"
OUTPUT_HEADER = "flight_software/Core/Inc/ai_model.h"

def main():
    print("==================================================")
    print("      VELA FLIGHT - EDGE AI MODEL TRAINING        ")
    print("==================================================")

    if not os.path.exists(CSV_FILE):
        print(f"[ERROR] Dataset {CSV_FILE} not found. Run Ground Station first!")
        sys.exit(1)

    temps, rads, atts = [], [], []

    # 1. Ingest Dataset
    with open(CSV_FILE, 'r') as f:
        reader = csv.reader(f)
        header = next(reader, None)
        for row in reader:
            if len(row) >= 4:
                try:
                    temps.append(float(row[1]))
                    rads.append(float(row[2]))
                    atts.append(float(row[3]))
                except ValueError:
                    continue

    if len(temps) < 10:
        print("[ERROR] Not enough data to train. Let GDS run longer.")
        sys.exit(1)

    # 2. Train Model (Calculate Normal Operational Envelope)
    def calc_stats(data):
        mean = sum(data) / len(data)
        variance = sum((x - mean) ** 2 for x in data) / len(data)
        std_dev = variance ** 0.5 if variance > 0 else 0.0001
        return mean, std_dev

    t_mean, t_std = calc_stats(temps)
    r_mean, r_std = calc_stats(rads)
    a_mean, a_std = calc_stats(atts)

    print(f"[TRAINING] Ingested {len(temps)} telemetry records.")
    print(f"[MODEL] Temperature Bounds: {t_mean:.2f} +/- {t_std*3:.2f}")
    print(f"[MODEL] Radiation Bounds:   {r_mean:.2f} +/- {r_std*3:.2f}")
    print(f"[MODEL] Attitude Bounds:    {a_mean:.2f} +/- {a_std*3:.2f}")

    # 3. Export to C Header for STM32
    c_code = f"""/**
 * @file ai_model.h
 * @brief AUTO-GENERATED EDGE AI WEIGHTS
 * Trained on {len(temps)} telemetry samples.
 */

#ifndef AI_MODEL_H
#define AI_MODEL_H

/* Learned Means */
#define AI_TEMP_MEAN {t_mean}f
#define AI_RAD_MEAN {r_mean}f
#define AI_ATT_MEAN {a_mean}f

/* Learned Standard Deviations */
#define AI_TEMP_STD {t_std}f
#define AI_RAD_STD {r_std}f
#define AI_ATT_STD {a_std}f

/* Anomaly Threshold (3 Sigma = 99.7% confidence) */
#define AI_Z_THRESHOLD 3.0f

#endif /* AI_MODEL_H */
"""
    os.makedirs(os.path.dirname(OUTPUT_HEADER), exist_ok=True)
    with open(OUTPUT_HEADER, 'w') as f:
        f.write(c_code)

    print(f"[SUCCESS] AI Model weights exported to {OUTPUT_HEADER}")

if __name__ == "__main__":
    main()