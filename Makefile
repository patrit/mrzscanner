CXXFLAGS := --std=c++17 -g -I.
LDFLAGS=-g
LDLIBS=-ltesseract -llept -lPocoNet -lPocoUtil -lPocoFoundation -lPocoJSON


SRCS=src/Server.cpp src/Mrz.cpp src/MrzCleaner.cpp src/MrzValid.cpp src/Tesseract.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: server

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

server: files.inc $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

.PHONY: files.inc
files.inc:
	find static -type f -size +1k | xargs gzip -k -f && \
	(for i in $$(find static -type f); \
	do \
		echo -n '{"'$$i'", "' && \
		python3 -c "import mimetypes; print(mimetypes.guess_type('$$i')[0], end='')" && \
		echo '"},'; \
	done) > $@

src/Server.cpp: files.inc

clean:
	rm -f src/*.o server files.inc static/*.gz

format:
	clang-format -i *.hpp *.cpp

testab:
	ab -p download.json -T application/json -c 4 -n 100 http://localhost:9090/mrz

testsingle:
	curl -X POST -H "Content-Type: application/json" -H "Accept-Content: application/json" -d @download.json http://localhost:9090/mrz
