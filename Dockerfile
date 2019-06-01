#FROM alpine:3.9.4 as builder
#RUN apk add --no-cache -X http://dl-cdn.alpinelinux.org/alpine/edge/testing \
#  tesseract-ocr-dev make g++ poco-dev python3
FROM ubuntu:18.04 as builder
RUN apt-get update && apt-get install -y libtesseract-dev make g++ libpoco-dev python3
RUN mkdir /app
WORKDIR /app
COPY . /app
RUN make -j4
RUN ./copydeps.sh server

#FROM alpine:3.9.4
FROM ubuntu:18.04
WORKDIR /app
COPY --from=builder /app/*.so* /usr/lib/
COPY --from=builder /app/server server
COPY --from=builder /app/tesseract tesseract
COPY --from=builder /app/static static
CMD ["./server"]
