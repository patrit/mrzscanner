import requests

class Test_MRZ():

    def req(self, data):
        return requests.post("http://localhost:8080/mrz/api/v1/analyze_mrz",
                            json={"text_data": data})

    def test_valid_TD1(self):
        data = "IDD<<T220001293<<<<<<<<<<<<<<<\n6408125<2010315D<<<<<<<<<<<<<4\nMUSTERMANN<<ERIKA<PAULA<ANNA<<"
        resp = self.req(data)
        assert 200 == resp.status_code
        print(resp.content)
        ret = resp.json()
        assert "TD1" == ret["mrz_type"]
        assert "ID" == ret["type"]
        assert "" == ret["sex"]
        assert "ERIKA PAULA ANNA" == ret["names"]
        assert "MUSTERMANN" == ret["surname"]
        assert "T22000129" == ret["number"]
        assert "D" == ret["country"]
        assert "D" == ret["nationality"]
        assert "640812" == ret["date_of_birth"]
        assert "201031" == ret["expiration_date"]



    def test_valid(self):
        data = "PASGPNG<<LINDA<ZEE<ZEE<<<<<<<<<<<<<<<<<<<<<<\nX4000140D5SGP7706275F1008232S7788888H<<<<<94"
        resp = self.req(data)
        assert 200 == resp.status_code, resp.content
        ret = resp.json()
        assert "TD3" == ret["mrz_type"]
 
    def test_invalid(self):
        data = "AOCOGBABA<<MACAIRE<PIERRE<<<<<<<<<<<<<<<<<<<\nS0020010<7COG3008258M1212020<<<<<<<<<<<<<<<6"
        resp = self.req(data)
        assert 200 == resp.status_code, resp.content
        ret = resp.json()
        assert "TD3" == ret["mrz_type"]

    def test_invalid(self):
        data = "PTPOLKOWALCZYKIEWICZ<<ELWIRA<KATARZYNA<<<<<<\nYA80000006POL8211192F03112818181818181818183"
        resp = self.req(data)
        assert 400 == resp.status_code, resp.content

