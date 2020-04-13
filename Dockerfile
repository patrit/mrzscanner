# BUILDER
FROM ubuntu:21.04 as builder
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
RUN wget -q https://github.com/swagger-api/swagger-ui/archive/v3.48.0.tar.gz && \
  echo "ca6e2e96f0844fc0da4041a73071a526809951b59e3555595d4e2c9740508c1b  v3.48.0.tar.gz" | sha256sum -c && \
  rm -rf mrz/ui && \
  tar xvfz v3.48.0.tar.gz swagger-ui-3.48.0/dist && \
  mkdir -p mrz/ui && \
  cp swagger-ui-3.48.0/dist/* mrz/ui/ && \
  sed -i "s#https://petstore.swagger.io/v2/swagger.json#/mrz/api/v1/spec.json#g" mrz/ui/index.html && \
  rm mrz/ui/*.map
# compress static files
RUN find mrz -type f -size +1k | grep -v "\.gz$$" | xargs gzip -k -f

# RUNTIME
FROM ubuntu:21.04
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
      libtesseract4 \
      libpoconet70 libpocojson70 libpocofoundation70 libpocoutil70 && \
      rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY --from=builder /app/server server
COPY --from=builder /app/tesseract tesseract
COPY --from=builder /app/mrz mrz

EXPOSE 8080
CMD ["./server"]
