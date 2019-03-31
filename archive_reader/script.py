import sys
import datetime
import sqlite3
import tarfile
import json
import itertools
import os
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.dates as mdates
import ipdb
import numpy
import datetime
from matplotlib import dates

cvt_time = np.vectorize(lambda t: dates.date2num(datetime.datetime.fromtimestamp(t)))

class TarMember:
    def __init__(self, tar, member):
        self.f = tar.extractfile(member)

    def __enter__(self):
        return self.f

    def __exit__(self, exc_type, exc_value, exc_traceback):
        self.f.close()

def files_in_directory(dname, recurse=True):
    """returns generator"""
    return (os.path.join(triple[0], fname) for triple in os.walk(dname) for fname in triple[2]) if recurse else (os.walk(dname).next()[2])

def tar_members(fname):
    with tarfile.open(fname, debug=3, errorlevel=2) as tar:
        for member in tar:
            if member.isfile():
                with TarMember(tar, member) as memberfile:
                    yield member, memberfile

def tar_member_json_objects(fname):
    for _, f in tar_members(fname):
        yield json.load(f)

def dir_file_json_objects(dname):
    for f in os.listdir(dname):
        yield json.load(f)

def orders(dname):
    for root in dir_file_json_objects(dname):
        for order in root['items']:
            yield order

def tar_orders(fname):
    for root in tar_member_json_objects(fname):
        for order in root['items']:
            yield order

def dir_tar_orders(dname, recurse=True):
    for fname in files_in_directory(dname, recurse):
        for order in tar_orders(fname):
            yield order

def trit_orders_from_tar(f, data=dict()):
    for order in orders(f):
        if order['type'] == 34 and not order['buy']:
            price = order['price']
            volume = order['volume']
            if data.has_key(price):
                data[price] += volume
            else:
                data[price] = volume
    return data

def files(dname, recurse=True):
    for tarfname in files_in_directory(dname, recurse):
        viewtime = '{}-{}-{}T{}:{}:{}'.format(*os.path.basename(tarfname).split('.')[0].split('_'))
        for _, f in tar_members(tarfname):
            yield viewtime, f.read()

_range_to_int = {
            '1': 1,
            '2': 2,
            '3': 3,
            '4': 4,
            '5': 5,
            '10': 10,
            '20': 20,
            '30': 30,
            '40': 40,
            'region': -1,
            'solarsystem': -2,
            'station': -3
    }

def parse_time(t_str):
    year = int(t_str[0:4])
    month = int(t_str[5:7])
    day = int(t_str[8:10])
    hour = int(t_str[11:13])
    minute = int(t_str[14:16])
    second = int(t_str[17:19])
    return (datetime.datetime(year, month, day, hour, minute, second) - datetime.datetime(1970,1,1)).total_seconds()

def to_list(order):
    buy = int(order["buy"])
    issued = parse_time(order['issued'])
    price = order['price']
    volume = order['volume']
    duration = order['duration']
    id = order['id']
    minVolume = order['minVolume']
    volumeEntered = order['volumeEntered']
    range = _range_to_int[order['range']]
    stationID = order['stationID']
    type = order['type']

    return [buy, issued, price, volume, duration, id, minVolume, volumeEntered, range, stationID, type]

def views(dname):
    for fname in sorted(files_in_directory(dname)):
        viewtime, e = os.path.basename(fname).split('.')
        if e == 'npz':
            yield viewtime, fname

def load_npzs(dname):
    for viewtime, fname in views(dname):
        try:
            yield viewtime, np.load(fname)['arr_0']
        except Exception as e:
            print e

#def load_npzs(dname):
#    for fname in sorted(files_in_directory(dname)):
#        viewtime, e = os.path.basename(fname).split('.')
#        if e != 'npz':
#            continue
#        sys.stdout.write(fname + ': ')
#        try:
#            yield viewtime, np.load(fname)['arr_0']
#            print 'Done'
#        except Exception as e:
#            print e

def load_npz(fname):
    with np.load(fname) as data:
        for viewtime in sorted(data.keys()):
            arr = data[viewtime]
            yield parse_time(viewtime), arr

#def fix_indices(dname):
#def fix_index(fname):
#    with open(fname) as f:
#        for line in f:
#

# failed
#   /tmp/home/racko/market_data/2017_04_21_23_55_07.tgz: Expecting , delimiter: line 1 column 4608514 (char 4608513)
#     - 6.json
#     - tail -c+4608400  2017_04_21_23_55_07/6.json | head -c200 # NOTE volumeEntered
#       11T10:31:05", "price": 49249.0, "volume": 2, "duration": 90, "id": 4836733827, "minVolume": 1, "volumeEntered": 2 22010674.5, "volume": 2, "duration": 90, "id": 4836733550, "minVolume": 1, "volumeEnte
#   /tmp/home/racko/market_data/2017_04_17_03_25_08.tgz: missing key 'volume'
#     - 1.json
#     - {u'issued': u'2017-03-21T11:13:01', u'price': 5000060000391, u'buy': False, u'type': 37413}, bug in the data I got from the server

# 1. dir_to_npy -> folder of npzs (one per viewtime). Can be read using load_npzs
# 2. multiple_npzs_to_single_npz -> single npz ("archive" with one array per viewtime). Uses load_npzs. Can be read using load_npz
# 3. single_npz_to_single_npz_per_type -> folder of npys (one per type). Can be read using (e.g.) read_single_type.

def dir_to_npy(dname, recurse=True, compress=True):
    for fname in sorted(files_in_directory(dname, recurse)):
        b, e = os.path.basename(fname).split('.')
        if e != 'tgz':
            continue
        sys.stdout.write(fname + ': ')
        try:
            d = os.path.dirname(fname) # files_in_directory recurses, so d may be longer than dname
            p = os.path.join(d, b + '.npz')
            if not os.path.isfile(p):
                if compress:
                    np.savez_compressed(p, np.array([to_list(order) for order in tar_orders(fname)]))
                else:
                    np.savez(p, np.array([to_list(order) for order in tar_orders(fname)]))
                print 'Done'
            else:
                print 'Already done'
        except Exception as e:
            print e

def read_single_type(dname, typeid):
    with open(os.path.join(dname, str(typeid) + '.index')) as index, open(os.path.join(dname, str(typeid) + '.npy')) as data:
        for line in index:
            # each np.load reads one array from data. data contains as many
            # arrays as index has lines, but we have to ignore the final
            # newline.
            yield float(parse_time(line.split(',')[1][:-1])), np.load(data)

def single_npz_to_single_npz_per_type(fname):
    p = os.path.dirname(fname)
    files = dict()
    indices = dict()
    for viewtime, x in load_npz(fname):
        ix = np.argsort(x[:,-1])
        by_type = np.split(x[ix], np.where(np.diff(x[ix,-1]) != 0)[0]+1)
        for orders in by_type:
            typeid = int(orders[0,-1])
            if not typeid in files:
                files[typeid] = open(os.path.join(p, str(typeid) + '.npy'), 'w')
                indices[typeid] = open(os.path.join(p, str(typeid) + '.index'), 'w')
            indices[typeid].write('{},{}\n'.format(files[typeid].tell(), viewtime))
            np.save(files[typeid], orders)
    for f in indices.values():
        f.close()
    for f in files.values():
        f.close()

def multiple_npzs_to_single_npz_per_type(dname):
    files = dict()
    indices = dict()
    for viewtime, x in load_npzs(dname):
        sys.stdout.write("{}: ".format(viewtime))
        ix = np.argsort(x[:,-1])
        by_type = np.split(x[ix], np.where(np.diff(x[ix,-1]) != 0)[0]+1)
        for orders in by_type:
            typeid = int(orders[0,-1])
            if not typeid in files:
                files[typeid] = open(os.path.join(dname, str(typeid) + '.npy'), 'w')
                indices[typeid] = open(os.path.join(dname, str(typeid) + '.index'), 'w')
            indices[typeid].write('{},{}\n'.format(files[typeid].tell(), viewtime))
            np.save(files[typeid], orders)
        print "Done"
    for f in indices.values():
        f.close()
    for f in files.values():
        f.close()


class SortedMapping:
    def __init__(self, iterable):
        self.data = dict(iterable)
    def keys(self):
        return sorted(self.data.keys())
    def __getitem__(self, key):
        return self.data[key]

class LazyNpzDirectoryLoader:
    def __init__(self, dname):
        self.views = dict(views(dname))
    def keys(self):
        return sorted(self.views.keys())
    def __getitem__(self, key):
        return np.load(self.views[key])['arr_0']

def savez_lazy(fname, mapping):
    with open(fname, 'w') as f:
        for viewtime, x in mapping:
            np.save(f, x)

def multiple_npzs_to_single_npz(dname, compress=False):
    """
    Problem: **mapping will read all arrays in dname into RAM :/
    """
    p = dname + '.npz'
    if os.path.isfile(p):
        raise Exception(p + ' already exists.')
    #mapping = SortedMapping(load_npzs(dname))
    mapping = LazyNpzDirectoryLoader(dname)
    if compress:
        np.savez_compressed(p, **mapping)
    else:
        np.savez(p, **mapping)

def plot(orders):
    by_order = dict()
    for t, arr in orders:
        for i in xrange(arr.shape[0]):
            if arr[i,0] > 0.5:
                continue
            oid = arr[i,5]
            if not oid in by_order:
                by_order[oid] = []
            by_order[oid].append((t, arr[i]))
    t0 = min(_[0] for _ in orders)
    plt.figure(1)
    plt.title('price')
    plt.figure(2)
    plt.title('volume')
    for oid, data in by_order.iteritems():
        t = np.array([_[0] for _ in data])
        ix = np.argsort(t)
        data = np.array([_[1] for _ in data])
        plt.figure(1)
        plt.plot((t[ix] - t0) / 3600.0 / 24.0, data[ix,2], label=str(oid))
        plt.figure(2)
        plt.plot((t[ix] - t0) / 3600.0 / 24.0, data[ix,3], label=str(oid))
    plt.figure(1)
    plt.legend()
    plt.figure(2)
    plt.legend()
    plt.show()

def plot_against_time():
    fig = plt.figure()
    ax = fig.add_subplot(111)
    fig.autofmt_xdate()
    ax.xaxis.set_major_formatter(mdates.DateFormatter('%d-%m-%y %H:%M'))
    return fig, ax

def order_counts(orders):
    t = np.array([x[0] for x in orders])
    sell_count = [(sample[:,0] < 0.5).sum() for _, sample in orders]
    buy_count = [(sample[:,0] > 0.5).sum() for _, sample in orders]
    #t = (t - t.min()) / 3600.0 / 24.0
    t = cvt_time(t)
    fig, ax = plot_against_time()
    ax.set_title('order count (not volume)')
    ax.plot(t, sell_count, '.', label='sell')
    ax.plot(t, buy_count, '.', label='buy')
    ax.legend()
    fig.show()

def plot_min_buy_max_sell(orders):
    t = np.array([x[0] for x in orders])

    min_ix = [np.where(sample[:,0] < 0.5)[0][np.argmin(sample[sample[:,0] < 0.5,2])] for _, sample in orders]
    min_sell = np.array([x[1][i] for x, i in zip(orders, min_ix)])

    max_ix = [np.where(sample[:,0] > 0.5)[0][np.argmax(sample[sample[:,0] > 0.5,2])] for _, sample in orders]
    max_buy = np.array([x[1][i] for x, i in zip(orders, max_ix)])

    #t = (t - t.min()) / 3600.0 / 24.0
    t = cvt_time(t)
    fig, ax = plot_against_time()
    ax.set_title('price')
    ax.plot(t, min_sell[:,2], '.', label='min sell')
    ax.plot(t, max_buy[:,2], '.', label='max buy')
    ax.legend()

    fig, ax = plot_against_time()
    ax.set_title('volume')
    ax.plot(t, min_sell[:,3], '.', label='min sell')
    ax.plot(t, max_buy[:,3], '.', label='max buy')
    ax.legend()

    fig, ax = plot_against_time()
    ax.set_title('id')
    ax.plot(t, min_sell[:,5], '.', label='min sell')
    ax.plot(t, max_buy[:,5], '.', label='max buy')
    ax.legend()
    fig.show()

def filter_orders_by_type(orders, typeid):
    for t, sample in orders:
        if sample.shape[0] % 30000 != 0:
            yield (t, sample[np.isclose(sample[:,10], typeid)])

def gnosis_orders():
    return filter_orders_by_type(load_npzs('/cifs/server/Media/Tim/market_data/2016/11/'), 3756)

# TODO: convert dir orders to matrix, write to .npy (maybe use cStringIO and store in sqlite)
# t = datetime.datetime.strptime('2016-12-06T11:10:02', '%Y-%m-%dT%H:%M:%S')
# (t - datetime.datetime(1970,1,1)).total_seconds() == 1481022602.0
# region, solarsystem, station, number of jumps (1, 2, 3, 4, 5, 10, 20, 30, 40)
# set(order['range'] for order in script.orders('/cifs/server/Media/Tim/market_data/2016/06/14/2016_06_14_21_20_43.tgz'))

if __name__ == "__main__":
    conn = sqlite3.connect('/cifs/server/Media/Tim/orders.sqlite')
    conn.execute("CREATE temp TABLE big(viewtime text, json JSON);")
    dname = '/cifs/server/Media/Tim/market_data/2016/06/14'
    conn.executemany("insert into big values(?,?)", files(dname))
    conn.execute("insert into orders select json_extract(row.value, '$.buy') as buy, json_extract(row.value, '$.issued') as issued, json_extract(row.value, '$.price') as price, json_extract(row.value, '$.volume') as volume, json_extract(row.value, '$.duration') as duration, json_extract(row.value, '$.id') as id, json_extract(row.value, '$.minVolume') as minVolume, json_extract(row.value, '$.volumeEntered') as volumeEntered, json_extract(row.value, '$.range') as range, json_extract(row.value, '$.stationID') as stationID, json_extract(row.value, '$.type') as type, viewtime from big, json_each(big.json, '$.items') as row;")
    #cursor = conn.execute("select * from big;")
    #data = trit_orders_from_tar('/cifs/server/Media/Tim/market_data/2016/06/14/2016_06_14_21_20_43.tgz')
    #data = np.array(data.items())
    #data = data[data[:,0].argsort()]
    #plt.plot(np.cumsum(data[:,1]), np.cumsum(data[:,0] * data[:,1]), '.')
    #plt.plot(np.cumsum(data[:,1]), data[:,0], '.-')

    #print 'min: {}'.format(min(data.keys()))
    #print 'max: {}'.format(max(data.keys()))
    #plt.plot(data.keys(), data.values(), '.')
    #plt.show()
