import os
import glob
import yaml
from os.path import join
import sqlite3
import untangle

def load_regions(path):
    def regions():
        print "regions:"
        for region in os.listdir(path):
            with open(join(path, region, "region.staticdata")) as f:
                root = yaml.safe_load(f)
                id = root["regionID"]
                print "{}: {}".format(region, id)
                yield region, id
    return regions()

def load_constellations(path):
    def constellations():
        print "constellations:"
        for region in os.listdir(path):
            region_path = join(path, region)
            with open(join(region_path, "region.staticdata")) as regionfile:
                regionroot = yaml.safe_load(regionfile)
                regionid = regionroot["regionID"]
                for constellation in os.listdir(region_path):
                    constellation_path = join(region_path, constellation)
                    if not os.path.isdir(constellation_path):
                        continue
                    with open(join(constellation_path, "constellation.staticdata")) as constellationfile:
                        constellationroot = yaml.safe_load(constellationfile)
                        constellationid = constellationroot["constellationID"]
                        print "{}, {}: {}".format(region, constellation, constellationid)
                        yield constellationid, constellation, regionid
    return constellations()

def load_systems(path):
    def systems():
        print "systems:"
        for region in os.listdir(path):
            region_path = join(path, region)
            with open(join(region_path, "region.staticdata")) as regionfile:
                regionroot = yaml.safe_load(regionfile)
                regionid = regionroot["regionID"]
                for constellation in os.listdir(region_path):
                    constellation_path = join(region_path, constellation)
                    if not os.path.isdir(constellation_path):
                        continue
                    with open(join(constellation_path, "constellation.staticdata")) as constellationfile:
                        constellationroot = yaml.safe_load(constellationfile)
                        constellationid = constellationroot["constellationID"]
                        for system in os.listdir(constellation_path):
                            system_path = join(constellation_path, system)
                            if not os.path.isdir(system_path):
                                continue
                            with open(join(system_path, "solarsystem.staticdata")) as systemfile:
                                systemroot = yaml.safe_load(systemfile)
                                systemid = systemroot["solarSystemID"]
                                print "{}, {}, {}: {}".format(region, constellation, system, systemid)
                                yield systemid, system, constellationid
    return systems()

def load_stations(path):
    with open(path) as f:
        root = yaml.safe_load(f)
        for station in root:
            yield station["stationID"], station["stationName"], station["solarSystemID"]

def load_conquerable_stations(path):
    root = untangle.parse(path)
    for row in root.eveapi.result.rowset.row:
        stationid = int(row['stationID'])
        stationname = row['stationName']
        systemid = int(row['solarSystemID'])
        print "{}, {}: {}".format(systemid, stationname, stationid)
        yield stationid, stationname, systemid

def main():
    conn = sqlite3.connect('market_history.db')
    #universe_root = "/cifs/server/Media/Tim/Eve/SDE/sde/fsd/universe"
    #for space in ["eve", "wormhole"]:
    #    conn.executemany("insert or ignore into regions(name, regionid) values(?, ?)", load_regions(join(universe_root, space)))
    #    conn.executemany("insert or ignore into constellations(constellationid, name, regionid) values(?, ?, ?)", load_constellations(join(universe_root, space)))
    #    conn.executemany("insert or ignore into systems(systemid, name, constellationid) values(?, ?, ?)", load_systems(join(universe_root, space)))
    #conn.executemany("insert or ignore into stations(stationid, name, systemid) values(?, ?, ?);", load_stations("/cifs/server/Media/Tim/Eve/SDE/sde/bsd/staStations.yaml"))
    conn.executemany("insert or ignore into stations(stationid, name, systemid) values(?, ?, ?);", load_conquerable_stations("stations.xml"))
    conn.commit()

if __name__ == "__main__":
    main()
