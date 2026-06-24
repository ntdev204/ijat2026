#!/usr/bin/env python3
"""
dataset_split.py: Performs run-level splits (Train 70%, Val 15%, Test 15%)
without leaking target identifiers by grouping and splitting on run_id.
"""

import os
import sys
import argparse
import random
from pathlib import Path
import pandas as pd

def split_dataset(base_path, seed=42):
    base_path = Path(base_path).expanduser()
    index_file = base_path / "metadata" / "run_index.csv"
    
    if not index_file.exists():
        print(f"Error: run_index.csv not found at {index_file}. Run build_run_index.py first.", file=sys.stderr)
        return False
        
    df = pd.read_csv(index_file)
    if df.empty:
        print("Error: run_index.csv is empty.", file=sys.stderr)
        return False
        
    print(f"Loaded index containing {len(df)} runs.")
    
    train_runs = []
    val_runs = []
    test_runs = []
    
    groups = df.groupby(["environment", "scenario_id", "controller_id"])
    random.seed(seed)
    
    for (env, scenario, controller), group in groups:
        runs = list(group["run_id"].unique())
        random.shuffle(runs)
        
        n_runs = len(runs)
        if n_runs == 0:
            continue
            
        n_train = max(1, int(round(0.70 * n_runs)))
        n_val = int(round(0.15 * n_runs))
        n_test = n_runs - n_train - n_val
        
        if n_train + n_val + n_test != n_runs:
            n_test = n_runs - n_train - n_val
            
        group_train = runs[:n_train]
        group_val = runs[n_train:n_train+n_val]
        group_test = runs[n_train+n_val:]
        
        train_runs.extend(group["path"][group["run_id"].isin(group_train)].tolist())
        val_runs.extend(group["path"][group["run_id"].isin(group_val)].tolist())
        test_runs.extend(group["path"][group["run_id"].isin(group_test)].tolist())
        
    train_set = set(train_runs)
    val_set = set(val_runs)
    test_set = set(test_runs)
    
    leakage = train_set.intersection(val_set) or train_set.intersection(test_set) or val_set.intersection(test_set)
    if leakage:
        print(f"FAIL: Data leakage detected between splits! Common runs exist: {leakage}", file=sys.stderr)
        return False
        
    splits_dir = base_path / "metadata" / "splits"
    splits_dir.mkdir(parents=True, exist_ok=True)
    
    (splits_dir / "train_runs.txt").write_text("\n".join(train_runs) + "\n", encoding="utf-8")
    (splits_dir / "val_runs.txt").write_text("\n".join(val_runs) + "\n", encoding="utf-8")
    (splits_dir / "test_runs.txt").write_text("\n".join(test_runs) + "\n", encoding="utf-8")
    
    
    for split_name, run_paths in [("train", train_runs), ("val", val_runs), ("test", test_runs)]:
        print(f"Compiling consolidated parquet for split: {split_name}...")
        dfs = []
        for run_path in run_paths:
            p_file = Path(run_path) / "controller_samples.parquet"
            if p_file.exists():
                try:
                    df_run = pd.read_parquet(p_file)
                    dfs.append(df_run)
                except Exception as e:
                    print(f"Warning: Failed to read {p_file}: {e}")
        if dfs:
            df_split = pd.concat(dfs, ignore_index=True)
            output_parquet = splits_dir / f"{split_name}.parquet"
            df_split.to_parquet(output_parquet, index=False)
            print(f"Saved {len(df_split)} samples to {output_parquet}")
        else:
            print(f"No samples found to compile for split: {split_name}")
    
    print("\n" + "="*50)
    print("DATASET SPLIT COMPLETE")
    print("="*50)
    print(f"Seed used: {seed}")
    print(f"Train split: {len(train_runs)} runs ({len(train_runs)/len(df)*100:.1f}%)")
    print(f"Val split:   {len(val_runs)} runs ({len(val_runs)/len(df)*100:.1f}%)")
    print(f"Test split:  {len(test_runs)} runs ({len(test_runs)/len(df)*100:.1f}%)")
    
    print("\nScenario representation in splits (Train / Val / Test):")
    for scenario in df["scenario_id"].unique():
        tr_c = sum(1 for p in train_runs if scenario in p)
        v_c = sum(1 for p in val_runs if scenario in p)
        te_c = sum(1 for p in test_runs if scenario in p)
        print(f"  - {scenario:<30}: {tr_c} / {v_c} / {te_c}")
        
    print(f"\nSplit paths written to: {splits_dir.resolve()}")
    print("="*50 + "\n")
    return True

def main():
    parser = argparse.ArgumentParser(description="Perform stratified split of run dataset")
    parser.add_argument("--path", default="d:/Research/ijat2026/rai_ros2/src/rai_simulation/dataset", help="Dataset base path")
    parser.add_argument("--seed", type=int, default=42, help="Random seed for split repeatability")
    args = parser.parse_args()
    
    success = split_dataset(args.path, args.seed)
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main()
