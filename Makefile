SOURCES = main.cpp vcs.cpp objects.cpp index.cpp # List of source files add more here
OBJECTS = $(SOURCES:.cpp=.o)  # Object files corresponding to source files
TARGET = badgit # Output executable name

CXXFLAGS = -std=c++20 
CXX = clang++ 
LDFLAGS = -lssl -lcrypto 

# The default target to build
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $(TARGET)

# Rule to compile source files into object files
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up object files and the executable
clean:
	rm -f $(OBJECTS) $(TARGET)

