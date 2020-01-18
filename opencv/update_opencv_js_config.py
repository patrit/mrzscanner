#!/usr/bin/env python3

import re
import sys

CONFIG = "./platforms/js/opencv_js.config.py"
CVEXP = re.compile(r"cv\.([^ \(\)]*)[ \(\)]")

def makeWhiteList(lst):
    return lst

white_list = None
exec(open(CONFIG).read())
assert(white_list)

items_to_keep = []
for fn in sys.argv[1:]:
    with open(fn, "r") as fp:
        content = fp.read()
        items_to_keep += CVEXP.findall(content)

wl = []
for item in white_list:
    for k, v in item.items():
        item[k] = [x for x in v if x in items_to_keep]
    dic = {x: y for x, y in item.items() if y}
    if dic:
        wl.append(dic)
open(CONFIG, "w").write("white_list = makeWhiteList(" + str(wl) + ")")
