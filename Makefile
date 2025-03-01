CXX       := g++
CXXFLAGS  := -std=c++17 -Wall -Wextra -Iinclude
LDFLAGS   := -lboost_system -lpthread

TARGET    := proxy
SRCS      := src/main.cpp src/httprequest.cpp src/httpresponse.cpp src/cache.cpp
OBJS      := $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)
	rm -f $(OBJS)

src/%.o: src/%.cpp include/Logger.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
