import untangle
import sqlite3

def main():
    conn = sqlite3.connect('market_history.db')
    def regions():
            with open("stations.xml") as f:
                root = untangle.parse(f)
                for row in root.eveapi.result.rowset.row:
                    stationid = int(root.eveapi.result.rowset.row[0]['stationID'])
                    stationname = root.eveapi.result.rowset.row[0]['stationName']
                    solarsystemid = int(root.eveapi.result.rowset.row[0]['solarSystemID'])
                    yield stationid, stationname, solarsystemid
    conn.executemany("insert or ignore into regions(name, regionid) values(?, ?)", regions())
    conn.commit()

if __name__ == "__main__":
    main()
