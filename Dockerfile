#FROM alpine:3.10.1 as builder
#RUN apk add --no-cache \
#  -X http://dl-cdn.alpinelinux.org/alpine/edge/testing \
#  tesseract-ocr-dev make g++ poco-dev python3
FROM ubuntu:19.10 as builder
RUN apt-get update && apt-get install -y libtesseract-dev make g++ libpoco-dev python3
RUN mkdir /app
WORKDIR /app
COPY . /app
RUN make clean && make -j4

#FROM alpine:3.10.1
#RUN apk add --no-cache \
#  -X http://dl-cdn.alpinelinux.org/alpine/edge/testing \
#  tesseract-ocr poco
FROM ubuntu:19.10
RUN apt-get update && \
    apt-get install -y \
        libtesseract4 \
        libpocojson62 \
        libpoconet62 \
        libpocoutil62 && \
    apt-get clean && \
    rm -fr /var/lib/apt/lists/* /tmp/* /var/tmp/*
WORKDIR /app
COPY tesseract tesseract
COPY --from=builder /app/server server
COPY --from=builder /app/static static

EXPOSE 8080
CMD ["./server"]
