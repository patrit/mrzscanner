# MRZ Scanner

## Build
Build the docker image

```docker build -t mrzscanner .```

## Usage
Run the docker container

```docker run --rm  -p 8080:8080 mrzscanner```

Run test against server

```./client.py path/to/image.jpg```

## External resources
- MRZ Info on [Wikimedia](https://en.wikipedia.org/wiki/Machine-readable_passport)
- [ICAO Machine Readable Travel Documents (Doc 9303, Part 3)](https://www.icao.int/publications/Documents/9303_p3_cons_en.pdf)
- [Prado - Public Register of Authentic travel and identity Documents Online](https://www.consilium.europa.eu/prado)
- tesseract dictionary [mrz_fast.traineddata](https://github.com/Exteris/tesseract-mrz)
- tesseract dictionary [ocrb_int.traineddata](https://github.com/Shreeshrii/tessdata_ocrb)