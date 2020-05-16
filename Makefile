CXX = g++
REPOS = ~/Repos

SRC = $(wildcard *.cpp)
OBJS = $(SRC:.cpp=.o)

CXXFLAGS = -Wfatal-errors -I $(REPOS)/libEternal/include/
LDFLAGS = -lEternal -lSDL2 -lSDL2_image -lSDL2_mixer -lGL -lSDL2_net -lGLEW -lpthread -L $(REPOS)/libEternal/

out: $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	

.PHONY: clean

clean:
	rm -f $(OBJS) out
