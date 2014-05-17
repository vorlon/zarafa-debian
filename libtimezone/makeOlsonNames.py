#!/usr/bin/python

import xml.etree.ElementTree as ET
import sys

# windowsZones.xml is downloaded from http://cldr.unicode.org/index/downloads
tree = ET.parse(sys.argv[1])
root = tree.getroot()
zones = root.find('windowsZones')
map = zones.find('mapTimezones')
print '''#include "platform.h"
#include <map>
#include <string>
#include <boost/assign.hpp>
namespace ba = boost::assign;

// map olson name to windows name
std::map<std::string, std::wstring> mapWindowsOlsonNames = ba::map_list_of'''
mapping = {}
for child in map:
    windows = child.attrib['other']
    olson = child.attrib['type']
    for name in olson.split(' '):
        mapping[name] = windows
for tz in mapping.iterkeys():
    print '\t("%s", L"%s")' % (tz, mapping[tz])
print ';'
