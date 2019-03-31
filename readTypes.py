import os
import glob
import yaml
from os.path import join
import sqlite3
from tqdm import tqdm

def main():
    conn = sqlite3.connect('market_history.db')
    def types():
        with open("typeIDs.yaml") as f:
            root = yaml.safe_load(f)
            for k, v in tqdm(root.iteritems()):
                if v["name"]:
                    yield k, v["name"]["en"]

    conn.executemany("insert or ignore into types(typeid, name) values(?, ?)", types())
    conn.commit()

if __name__ == "__main__":
    main()
