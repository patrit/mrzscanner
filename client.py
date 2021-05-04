#!/usr/bin/env python3

import base64
import json
import requests
import sys
import time

if __name__ == "__main__":
    fname = sys.argv[1]
    with open(fname, "rb") as fptr:
        raw = fptr.read()
        raw = base64.b64encode(raw)
        data = {"image_data": raw.decode("utf-8")}
        start = time.time()
        ret = requests.post("http://localhost:8080/mrz/api/v1/analyze_image_mrz", json=data)
        print(time.time() - start)
        if ret.status_code != 200:
            ret2 = requests.post("http://localhost:8080/mrz/api/v1/analyze_image_mrz?debugonly=true", json=data)
            print(ret2.text)
            print(json.dumps(
                {
                    "status_code": ret.status_code,
                    "json": ret.json()
                }, indent=2
            )
        )
        assert ret.status_code == 200
