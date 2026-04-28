CXX = g++
CXXFLAGS = -O2 -std=c++17 -Wall -Wextra -fopenmp
TARGET = task_a_merge_sort

all: $(TARGET)

$(TARGET): task_a_merge_sort.cpp
	$(CXX) $(CXXFLAGS) task_a_merge_sort.cpp -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) task_a_results.csv task_a_results.png
