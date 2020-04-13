CXXFLAGS := --std=c++17 -O3 -flto -I.
LDFLAGS=-O3 -flto
LDLIBS=-ltesseract -llept -lPocoNet -lPocoUtil -lPocoFoundation -lPocoJSON -lstdc++fs


SRCS=src/Server.cpp src/Mrz.cpp src/MrzCleaner.cpp src/MrzValid.cpp src/MrzFields.cpp src/Tesseract.cpp src/Countries.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: server

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

server: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

clean:
	rm -f src/*.o server static/*.gz

test:
	pytest-3 -v

format:
	clang-format -i src/*.hpp src/*.cpp

testab:
	ab -p download.json -T application/json -c 4 -n 100 http://localhost:8080/mrz/api/v1/analyze_image

testsingle:
	curl -X POST -H "Content-Type: application/json" -H "Accept-Content: application/json" -d @download.json http://localhost:8080/mrz/api/v1/analyze_image
