import json
import sqlite3
import sys
from multiprocessing import Pool
import sso
import requests

class StructureFetcher:
    def __init__(self, token):
        self.token = token
    def __call__(self, structureid):
        try:
            stationinfo = sso.call_esi('https://esi.evetech.net/latest/universe/structures/{}/'.format(structureid), self.token)
        except Exception as e:
            print(e)
            return None
        else:
            stationname = stationinfo['name']
            solarsystemid = int(stationinfo['solar_system_id'])
            return structureid, stationname, solarsystemid

def main():
    token, _ = sso.refresh("esi-universe.read_structures.v1")
    pool = Pool(10)
    res = requests.get('https://esi.evetech.net/latest/universe/structures/')
    res.raise_for_status()
    structureids = res.json()
    structures = pool.map(StructureFetcher(token), structureids)
    print('error count: {}'.format(sum(1 for _ in structures if _ is None)))
    conn = sqlite3.connect('market_history.db')
    conn.executemany("insert or ignore into stations(stationid, name, systemid) values(?, ?, ?);", (_ for _ in structures if not _ is None))
    conn.commit()

if __name__ == "__main__":
    main()
