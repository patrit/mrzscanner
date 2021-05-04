import glob
import pytest
import requests
import base64
from PIL import Image, ImageOps
from io import BytesIO

def get_prado_images():
    with open("test/prado.txt", "r") as fp:
        items = [x.strip() for x in fp.readlines()]
        return sorted([x for x in items if len(x) > 0 and not x.startswith("#")])[:5]

def get_images():
    return sorted(glob.glob("test/images/*.jpg"))

class Test_OCR():

    def req(self, data):
        return requests.post("http://localhost:8080/mrz/api/v1/analyze_image_mrz",
                            json={"image_data": data})

    @pytest.mark.parametrize("imgurl", get_prado_images())
    def test_image(self, imgurl):
        # download image
        resp = requests.get(imgurl)
        assert 200 == resp.status_code
        data = base64.b64encode(resp.content).decode("utf-8")
        # call ocr
        resp = self.req(data)
        assert 200 == resp.status_code, resp.content
        ret = resp.json()
        assert ret["mrz_type"] in ("TD3",)


    @pytest.mark.parametrize("imgfn", get_images())
    def _test_image(self, imgfn):
        with open(imgfn, "rb") as fp:
            img = Image.open(fp)
            img = ImageOps.grayscale(img)
            data = BytesIO()
            img.save(data, format="PNG")
            data = data.getvalue()

            #data = fp.read()
            data = base64.b64encode(data)
            # call ocr
            resp = self.req(data)
            assert 200 == resp.status_code, resp.content
            ret = resp.json()
            assert ret["mrz_type"] in ("TD3", "TD1")
