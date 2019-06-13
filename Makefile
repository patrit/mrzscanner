CXXFLAGS := --std=c++17 -O3 -flto -I.
LDFLAGS=-O3 -flto
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
	find static -type f -size +1k | grep -v ".gz$$" | xargs gzip -k -f && \
	(for i in $$(find static -type f); \
	do \
		echo -n '{"'/$$i'", "' && \
		python3 -c "import mimetypes; print(mimetypes.guess_type('$$i')[0], end='')" && \
		echo '"},'; \
	done) > $@

src/Server.cpp: files.inc

clean:
	rm -f src/*.o server files.inc static/*.gz

format:
	clang-format -i src/*.hpp src/*.cpp

testab:
	ab -p download.json -T application/json -c 4 -n 100 http://localhost:8080/mrz

testsingle:
	curl -X POST -H "Content-Type: application/json" -H "Accept-Content: application/json" -d @download.json http://localhost:8080/mrz
