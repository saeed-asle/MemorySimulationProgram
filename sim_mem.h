/*Ex5
ID:315957399
SAEED ASLE
*/
#ifndef SIM_MEM_H
#include <ostream>
#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
using namespace std;
#define MEMORY_SIZE 200
extern char main_memory[MEMORY_SIZE];
#define SIM_MEM_H
struct page_descriptor;
class sim_mem {
    int swapfile_fd; //swap file fd
    int program_fd; //executable file fd
    int text_size;
    int data_size;
    int bss_size;
    int heap_stack_size;
    int num_of_pages;
    int page_size;
    int num_of_proc;
    int **main_memory_helper;

    page_descriptor **page_table; //pointer to page table
    public:
        sim_mem(char exe_file_name[], char swap_file_name[], int text_size, int data_size,
        int bss_size, int heap_stack_size, int page_size);
        /**/
        ~sim_mem();
        char load(int address);
        void store(int address, char value);
        void print_memory();
        void print_swap ();
        void print_page_table();
        void search();
        void delete_();
        void new_page(int *out, int *in ) ;
};
typedef struct page_descriptor{
bool valid;
int frame;
bool dirty;
int swap_index;
} page_descriptor;
#endif


