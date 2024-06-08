
#include "sim_mem.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>
#include <unistd.h>
using namespace std;
char main_memory[MEMORY_SIZE];


sim_mem::sim_mem(char exe_file_name[],char swap_file_name[],int text_size,int data_size,
                                                            int bss_size,int heap_stack_size,int page_size) {
   this->text_size=text_size;
   this->data_size=data_size;
   this->bss_size=bss_size;
   this->heap_stack_size=heap_stack_size;
   this->page_size=page_size;
   this->num_of_pages=(data_size+bss_size+heap_stack_size)/page_size;
   this->num_of_proc=-1;
    // Open the swap file
   if (exe_file_name == nullptr){
      std::cerr << "Got empty name!\n" << std::endl;
      exit(1);
   }
   if (swap_file_name == nullptr){
      std::cerr << "Got empty name!\n" << std::endl;
      exit(1);
   }
   this->swapfile_fd = open(swap_file_name, O_RDWR | O_CREAT, 0666);
   if (swapfile_fd < 0) {
      std::cerr << "Error opening swap file." << std::endl;
      exit(1);
   }
    // Open the executable file
   this->program_fd = open(exe_file_name, O_RDONLY);
   if (program_fd < 0) {
      std::cerr << "Error opening executable file." << std::endl;
      exit(1);
   }
   memset(main_memory, 0, sizeof(main_memory));
    // hhelps to find places in page table
   this->main_memory_helper=new int*[MEMORY_SIZE/page_size];
   for (int i = 0; i < MEMORY_SIZE/page_size; i++) {
      this->main_memory_helper[i]=new int[2];
      for (int j = 0; j < 2; j++) {
         this->main_memory_helper[i][j]=-1;
      }
   }
   this->page_table=new page_descriptor*[4];
   for (int i = 0; i < 4; i++) {//make malloc with the sizees thhat thhe user input for evry place
      if (i == 0)
         page_table[i] = new page_descriptor[text_size / page_size];
      else if (i == 1)
         page_table[i] = new page_descriptor[data_size / page_size];
      else if (i == 2)
         page_table[i] = new page_descriptor[ bss_size / page_size];
      else
         page_table[i] = new page_descriptor[heap_stack_size / page_size];
      int num=0;
      if (i == 0)
         num=text_size / page_size;
      else if (i == 1)
         num=data_size / page_size;
      else if (i == 2)
         num=bss_size / page_size;
      else
         num=heap_stack_size / page_size;         

      for (int j = 0; j < num; j++) {
         page_table[i][j].valid = false;
         page_table[i][j].frame = -1;
         page_table[i][j].dirty = false;
         page_table[i][j].swap_index = -1;
      }
   }
   for (int i = 0; i < (data_size + bss_size + heap_stack_size); i++) {
      char c = '0';//fill withh zeros
      if (write(this->swapfile_fd, &c, sizeof(c)) < 0) {
         std::cerr << "Error writing to swap file." << std::endl;
         exit(1);
      }
   }
}
sim_mem::~sim_mem() {
    // Close open files
   close(swapfile_fd);
   close(program_fd);
// Free main_memory_helper
   for (int i = 0; i < MEMORY_SIZE / page_size; i++) {
      delete[] main_memory_helper[i];
   }
   delete[] main_memory_helper;

   // Free page_table
   for (int i = 0; i < 4; i++) {
      delete[] page_table[i];
   }
   delete[] page_table;
}
std::string decimalToBinary(int decimal) {
   std::string binary;
   while (decimal > 0) {
      binary = (decimal % 2 == 0 ? "0" : "1") + binary;
      decimal /= 2;
   }
   if (binary.length() < 12) {
      binary = std::string(12 - binary.length(), '0') + binary;
   }
   return binary;
}
int binaryToDecimal(const std::string& binary) {
   int decimal = 0;int power = 0;
   for (int i = binary.length() - 1; i >= 0; i--) {
      if (binary[i] == '1') {
         decimal += pow(2, power);
      }
      power++;
   }
   return decimal;
}
char sim_mem::load(int address) {
   string binary=decimalToBinary(address);
   if(binary.length()>12){
      perror("number out of range");
      return '\0';
   }
   int page=binaryToDecimal(binary.substr(0,12-log2(this->page_size)));
   int out=binaryToDecimal(binary.substr(0,2));
   int frame=binaryToDecimal(binary.substr(2,10));
   int in=frame/this->page_size;
   int offset=frame%this->page_size;
   int page_num = (int) address / page_size;
   if((out==0 && in>=text_size/page_size) ||(out==1 && in>=data_size/page_size)||(out==2 && in>=bss_size/page_size)||(out==3 && in>=heap_stack_size/page_size) ){
     printf("invald address :number out of range\n");
     return '\0';
   }
   else if(this->page_table[out][in].valid==true){//in main main_memory
      return main_memory[this->page_table[out][in].frame*this->page_size+offset];
   }
   else if(this->page_table[out][in].dirty==true){//in swap file
      search();
      lseek(swapfile_fd, 0, SEEK_SET);
      lseek(swapfile_fd ,this->page_table[out][in].swap_index  , SEEK_SET);
      read(swapfile_fd , main_memory+(num_of_proc * page_size), page_size);
      main_memory_helper[num_of_proc][0]=out;
      main_memory_helper[num_of_proc][1]=in;
      lseek(swapfile_fd, 0, SEEK_SET);
      lseek(swapfile_fd, this->page_table[out][in].swap_index, SEEK_SET);
      for (int i = 0; i < page_size; i++) {
         char c = '0';
         if (write(swapfile_fd, &c, sizeof(c)) < 0) {
            std::cerr << "Error writing to swap file." << std::endl;
            exit(1);
         }
      }
      page_table[out][in].valid = true;
      page_table[out][in].frame = num_of_proc;
      page_table[out][in].swap_index = -1;
      return main_memory[(num_of_proc * page_size) + offset];
   }
   else if(out==3){//heap stack error
      printf("doesn't exist\n");
         return '\0';
   }
   else{//calculte the place of the page and get from the file
      int veb=0;
      if (out==2) {
         veb=text_size+data_size+in*page_size;
      }
      else if(out==1){
         veb=text_size+in*page_size;
      }
      else{
         veb=in*page_size;
      }
      search();
      lseek(program_fd ,veb, SEEK_SET);
      read(program_fd , main_memory+(num_of_proc * page_size), page_size);
      page_table[out][in].valid = true;
      page_table[out][in].frame = num_of_proc;
      main_memory_helper[num_of_proc][0]=out;
      main_memory_helper[num_of_proc][1]=in;
      return  main_memory[(num_of_proc*page_size) + offset] ;
   }
}
void sim_mem::store(int address, char value) {
   string binary=decimalToBinary(address);
   if(binary.length()>12){
      perror("number out of range");
      return ;
   }
   int page=binaryToDecimal(binary.substr(0,12-log2(this->page_size)));
   int out=binaryToDecimal(binary.substr(0,2));
   int frame=binaryToDecimal(binary.substr(2,10));
   int in=frame/this->page_size;
   int offset=frame%this->page_size;
   if((out==0 && in>=text_size/page_size) ||(out==1 && in>=data_size/page_size)||(out==2 && in>=bss_size/page_size)||(out==3 && in>=heap_stack_size/page_size) ){
     printf("invald address :number out of range");
     return ;
   }
   else if(out==0){
      cerr<<"text section:invald address"<<endl;
   }
   else if(page_table[out][in].valid==true) {//in main_memory
        main_memory[(page_table[out][in].frame * page_size) + offset] = value;
        page_table[out][in].dirty = true;
   }
   else if(this->page_table[out][in].dirty==true){//in sawp file
      search();
      lseek(swapfile_fd, 0, SEEK_SET);
      lseek(swapfile_fd ,this->page_table[out][in].swap_index  , SEEK_SET);
      read(swapfile_fd , main_memory+(num_of_proc * page_size), page_size);
      main_memory_helper[num_of_proc][0]=out;
      main_memory_helper[num_of_proc][1]=in;
      lseek(swapfile_fd, 0, SEEK_SET);
      lseek(swapfile_fd, this->page_table[out][in].swap_index, SEEK_SET);
      for (int i = 0; i < page_size; i++) {
         char c = '0';
         if (write(swapfile_fd, &c, sizeof(c)) < 0) {
            std::cerr << "Error writing to swap file." << std::endl;
            exit(1);
         }
      }
      page_table[out][in].valid = true;
      page_table[out][in].frame = num_of_proc;
      page_table[out][in].swap_index = -1;
      main_memory[(num_of_proc * page_size) + offset] = value;
   }
   else if (out>1) {//bss or head first time so new page
      new_page(&out, &in);
      main_memory[num_of_proc * page_size + offset] = value;
      page_table[out][in].dirty = 1;
   }
   else{
      search();
      lseek(program_fd ,text_size+in*page_size, SEEK_SET);
      read(program_fd , main_memory+(num_of_proc * page_size), page_size);
      page_table[out][in].valid = true;
      page_table[out][in].frame = this->num_of_proc;
      main_memory[(num_of_proc*page_size) + offset] = value;
      page_table[out][in].dirty = true;
      main_memory_helper[num_of_proc][0]=out;
      main_memory_helper[num_of_proc][1]=in;
   }
}
//if thhe main_memory put the data in swap if dirty else just make it ass null place
void sim_mem::delete_() {
   int out_m=this->main_memory_helper[this->num_of_proc][0];
   int in_m=this->main_memory_helper[this->num_of_proc][1];
   int location= page_table[out_m][in_m].frame;
   char c;
   int bytesRead;
   int index = 0;
   lseek(swapfile_fd, 0, SEEK_SET);
   while ((bytesRead = read(swapfile_fd, &c, sizeof(char))) > 0) {
      if (c == '0') {
         break;
      }
      index++;
   }
   if(page_table[out_m][in_m].dirty==true){
      lseek(swapfile_fd ,index, SEEK_SET);
      if(write(swapfile_fd , main_memory+num_of_proc*page_size,page_size) < 0){
         perror("cannot write to file!\n");
         page_table[out_m][in_m].swap_index = index;
      }
   }
   page_table[out_m][in_m].valid = false;
   page_table[out_m][in_m].frame = -1;
   out_m=-1;
   in_m=-1;
}//search place in the main_memory
void sim_mem::search() {
   bool place = false;
   this->num_of_proc+=1;
   if (num_of_proc >= MEMORY_SIZE/page_size){
      this->num_of_proc = 0;
      place = true;
   }
   if(place == true)
      delete_();
   else if(this->main_memory_helper[this->num_of_proc][0]!=-1){
            delete_();

   }
}
void sim_mem::new_page(int *out, int *in ) {
   search();
   for (int i = 0 ; i < page_size ; i++)
      main_memory[num_of_proc * page_size + i] = '0';
   page_table[*out][*in].frame = num_of_proc;
   page_table[*out][*in].valid = 1;
   main_memory_helper[num_of_proc][0]=*out;
   main_memory_helper[num_of_proc][1]=*in;
}
void sim_mem::print_memory() {
   int i;
   printf("\n Physical memory\n");
   for(i = 0; i < MEMORY_SIZE; i++) {
      printf("[%c]\n", main_memory[i]);
   }
}
void sim_mem::print_swap() {
   char* str = (char*)malloc(this->page_size *sizeof(char));
   int i;
   printf("\n Swap memory\n");
   lseek(swapfile_fd, 0, SEEK_SET); // go to the start of the file
   while(read(swapfile_fd, str, this->page_size) == this->page_size) {
      for(i = 0; i < page_size; i++) {
         printf("%d - [%c]\t", i, str[i]);
      }
      printf("\n");
   }
}
void sim_mem::print_page_table() {
   int i;
   int num_of_txt_pages = text_size / page_size;
   int num_of_data_pages = data_size / page_size;
   int num_of_bss_pages = bss_size / page_size;
   int num_of_stack_heap_pages = heap_stack_size / page_size;
   printf("Valid\t Dirty\t Frame\t Swap index\n");
   for(i = 0; i < num_of_txt_pages; i++) {
      printf("[%d]\t[%d]\t[%d]\t[%d]\n",
      page_table[0][i].valid,
      page_table[0][i].dirty,
      page_table[0][i].frame ,
      page_table[0][i].swap_index);
   }
   printf("Valid\t Dirty\t Frame\t Swap index\n");
   for(i = 0; i < num_of_data_pages; i++) {
      printf("[%d]\t[%d]\t[%d]\t[%d]\n",
      page_table[1][i].valid,
      page_table[1][i].dirty,
      page_table[1][i].frame ,
      page_table[1][i].swap_index);
   }
   printf("Valid\t Dirty\t Frame\t Swap index\n");
   for(i = 0; i < num_of_bss_pages; i++) {
      printf("[%d]\t[%d]\t[%d]\t[%d]\n",
      page_table[2][i].valid,
      page_table[2][i].dirty,
      page_table[2][i].frame ,
      page_table[2][i].swap_index);
   }
   printf("Valid\t Dirty\t Frame\t Swap index\n");
   for(i = 0; i < num_of_stack_heap_pages; i++) {
      printf("[%d]\t[%d]\t[%d]\t[%d]\n",page_table[3][i].valid,page_table[3][i].dirty, 
      page_table[3][i].frame , page_table[3][i].swap_index); 
   }
}
