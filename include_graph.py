#!/usr/bin/python2
import os.path
import re
import sys
import toposort

def is_header(fname):
    return fname.endswith('.h')

def dependencies(f, regex):
    """
    f: file object

    Attempts to find header names included via #include
    """
    for line in f:
        result = regex.match(line)
        if result:
            included_header_name = result.group(1)
            yield included_header_name

def dependency_set(f, regex):
    return set(dependencies(f, regex))

def include_dirs(paths):
    for path in paths:
        for dirpath, dirnames, _ in os.walk(path):
            if "include" in dirnames:
                yield os.path.join(dirpath, "include")

def headers(paths):
    for include_dir in include_dirs(paths):
        for dirpath, _, filenames in os.walk(include_dir):
            for fname in filenames:
                if is_header(fname):
                    fullpath = os.path.join(dirpath, fname)
                    yield os.path.relpath(fullpath, include_dir), fullpath

def toposort_input(paths, regex):
    dependency_map = dict()
    for relpath, fullpath in headers(paths):
            with open(fullpath) as f:
                dependency_map[relpath] = dependency_set(f, regex)
    return dependency_map

def main(paths, regex=re.compile('^#include ["<]([^"]+)[">]')):
    print 'digraph G {'
    deps = toposort_input(paths, regex)
    for header, includes in deps.items():
        if includes:
            print '    {} -> "{}"'.format(", ".join('"{}"'.format(include) for include in includes), header)
        else:
            print '    "{}"'.format(header)
    for level in toposort.toposort(deps):
        print '    {{ rank = same; {}; }}'.format("; ".join('"{}"'.format(header) for header in level))
    print '}'

if __name__ == "__main__":
    main(sys.argv[2:], re.compile(sys.argv[1]))
