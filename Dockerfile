# BUILDER
FROM ubuntu:19.10 as builder
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      libtesseract-dev wget \
      make g++ libpoco-dev python3 python3-pip python3-setuptools
WORKDIR /app
COPY . /app
RUN python3 -m pip install -r test/requirements.txt
RUN make clean && make -j4 && strip server
RUN ./server & pytest -v -x

# grab swagger ui
RUN wget -q https://github.com/swagger-api/swagger-ui/archive/v3.24.3.tar.gz && \
  echo "a048586fedb9e7432c22b54138f370e6529ed31b68f11d230149d6751edf1f53  v3.24.3.tar.gz" | sha256sum -c && \
  rm -rf mrz/ui && \
  tar xvfz v3.24.3.tar.gz swagger-ui-3.24.3/dist && \
  mkdir -p mrz/ui && \
  cp swagger-ui-3.24.3/dist/* mrz/ui/ && \
  sed -i "s#https://petstore.swagger.io/v2/swagger.json#/mrz/api/v1/spec.json#g" mrz/ui/index.html && \
  rm mrz/ui/*.map
# compress static files
RUN find mrz -type f -size +1k | grep -v "\.gz$$" | xargs gzip -k -f

# RUNTIME
FROM ubuntu:19.10
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      libtesseract4 \
      libpoconet62 libpocojson62 libpocofoundation62 libpocoutil62 && \
      rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=builder /app/server server
COPY --from=builder /app/tesseract tesseract
COPY --from=builder /app/mrz mrz

EXPOSE 8080
CMD ["./server"]
