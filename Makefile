CXX = g++
LOCAL_CXXFLAGS = -O3 -march=native -fopenmp -msse2 -DHAVE_SSE2 -std=c++11 -Wno-unused-result -Wno-cpp

TARGET = qr

SRC = $(TARGET).cpp
SFMT_SRC = SFMT/SFMT.c

$(TARGET): $(SRC) $(SFMT_SRC)
	$(CXX) $(SRC) $(SFMT_SRC) $(LOCAL_CXXFLAGS) -o $@
