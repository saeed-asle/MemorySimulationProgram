

#include <ostream>
#include <iostream>
#include "sim_mem.h"
using namespace std;


int main() {
    char exe_file_name[] = "program.txt";
    char swap_file_name[] = "swap.txt";
    int text_size = 16;
    int data_size = 16;
    int bss_size = 32;
    int heap_stack_size = 32;
    int page_size = 8;
    sim_mem memory(exe_file_name, swap_file_name, text_size, data_size, bss_size, heap_stack_size, page_size);

    memory.load(0);
    memory.load(8);
    memory.store(1024,'q');
    memory.store(1032,'q');
    memory.load(2048);
    memory.load(2056);
    memory.load(2064);
    memory.load(2072);
    memory.load(3072);
    memory.load(3080);
    memory.load(3088);
    memory.load(3096);
    memory.store(1024,'q');
    memory.store(3096,'q');
    memory.store(1033,'f');



    char value1 = memory.load(5);
    char value2 = memory.load(15);
    char value3 = memory.load(25);
    std::cout << "Value 1: " << value1 << std::endl;
    std::cout << "Value 2: " << value2 << std::endl;
    std::cout << "Value 3: " << value3 << std::endl;

    // Print memory, swap file, and page table
    memory.print_memory();
    memory.print_swap();
    memory.print_page_table();

    return 0;
}