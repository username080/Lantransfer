#!/usr/bin/env python3
import os
import sys
import time
import zipfile
import urllib.request
import logging
import subprocess
from pathlib import Path

# --- LANTRANSFER PREPARATION: Auto-install dependencies on the server ---
try:
    import polars as pl
except ImportError:
    print("Polars not found on server. Auto-installing dependencies via Lantransfer...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "--quiet", "--break-system-packages", "polars"])
    import polars as pl
# ------------------------------------------------------------------------

logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s: %(message)s')
logger = logging.getLogger(__name__)

BASE_URL = "https://data.binance.vision/data/spot/monthly/aggTrades"
SYMBOLS = ["BTCUSDT", "ETHUSDT", "XRPUSDT", "ADAUSDT", "BNBUSDT"]
YEARS = range(2019, 2024)
MONTHS = range(1, 13)

# --- LANTRANSFER PREPARATION: Target the server's cache directory natively ---
CACHE_DIR = os.environ.get("LANTRANSFER_CACHE_DIR", "/tmp/lantransfer_cache")
DATA_DIR = os.path.abspath(os.path.join(CACHE_DIR, "backtest/data"))
# -----------------------------------------------------------------------------

TEMP_DIR = os.path.join(DATA_DIR, "raw_ticks_temp")
DB_DIR = os.path.join(DATA_DIR, "tick_db")

os.makedirs(TEMP_DIR, exist_ok=True)
os.makedirs(DB_DIR, exist_ok=True)

def process_to_parquet(symbol: str, year: int, month: int):
    month_str = f"{month:02d}"
    filename = f"{symbol}-aggTrades-{year}-{month_str}.zip"
    csv_filename = filename.replace(".zip", ".csv")
    url = f"{BASE_URL}/{symbol}/{filename}"
    
    zip_path = os.path.join(TEMP_DIR, filename)
    csv_path = os.path.join(TEMP_DIR, csv_filename)
    
    # Target Parquet File (Hive Partitioned format)
    out_dir = os.path.join(DB_DIR, symbol, str(year))
    os.makedirs(out_dir, exist_ok=True)
    parquet_path = os.path.join(out_dir, f"{month_str}.parquet")
    
    if os.path.exists(parquet_path):
        logger.info(f"[{symbol}] {year}-{month_str} already exists in database. Skipping.")
        return

    # 1. Download
    try:
        logger.info(f"[{symbol}] Downloading {year}-{month_str}...")
        urllib.request.urlretrieve(url, zip_path)
    except urllib.error.HTTPError as e:
        if e.code == 404:
            logger.warning(f"[{symbol}] Data not found for {year}-{month_str} (404). Skipping.")
        else:
            logger.error(f"HTTP error {e.code} for {url}. Skipping.")
        return
    except Exception as e:
        logger.error(f"Failed to download {filename}: {e}. Skipping.")
        return

    # 2. Extract
    logger.info(f"[{symbol}] Extracting {filename}...")
    try:
        with zipfile.ZipFile(zip_path, 'r') as zip_ref:
            zip_ref.extractall(TEMP_DIR)
    except Exception as e:
        logger.error(f"Failed to extract {filename}: {e}. Skipping.")
        if os.path.exists(zip_path):
            os.remove(zip_path)
        return

    # 3. Convert to Parquet with Polars
    logger.info(f"[{symbol}] Compressing {csv_filename} into Binary Parquet using Polars...")
    try:
        t0 = time.time()
        # Read CSV with optimized schema to drop useless columns and use smaller memory footprint
        df = pl.read_csv(
            csv_path,
            has_header=True, # Binance data usually has header, if not, adjust logic
            # Explicitly name columns because older binance data has varying headers
            new_columns=["agg_trade_id", "price", "quantity", "first_trade_id", "last_trade_id", "timestamp", "is_buyer_maker"],
            dtypes={
                "price": pl.Float32,
                "quantity": pl.Float32,
                "timestamp": pl.Int64,
                "is_buyer_maker": pl.Boolean
            },
            columns=[1, 2, 5, 6] # Only keep price, qty, timestamp, is_buyer_maker
        )
        
        # Write highly compressed ZSTD Parquet file
        df.write_parquet(parquet_path, compression="zstd")
        
        elapsed = time.time() - t0
        original_size = os.path.getsize(csv_path) / (1024 * 1024)
        new_size = os.path.getsize(parquet_path) / (1024 * 1024)
        
        logger.info(f"[{symbol}] Parquet Created! Time: {elapsed:.2f}s | Size: {original_size:.1f}MB -> {new_size:.1f}MB")
        
    except Exception as e:
        logger.error(f"Failed to process {csv_filename} with Polars: {e}")
        if os.path.exists(parquet_path):
            os.remove(parquet_path)

    # 4. Cleanup
    if os.path.exists(zip_path):
        os.remove(zip_path)
    if os.path.exists(csv_path):
        os.remove(csv_path)


def main():
    logger.info(f"Starting Polars Binary Tick Database Builder into {DB_DIR}...")
    for symbol in SYMBOLS:
        for year in YEARS:
            for month in MONTHS:
                process_to_parquet(symbol, year, month)
    
    logger.info(f"All 5 years across {len(SYMBOLS)} coins downloaded and compiled into {DB_DIR}!")

if __name__ == "__main__":
    main()
