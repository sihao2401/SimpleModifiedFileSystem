#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<time.h>

struct superblock{
    int isize;//size of inode blocks = inodes / inodes per block
    int fsize;//first block can not be allocated for file(size of file)
    int nfree;//the number of free block in free list
    int free[100]; //free block list
    int ninode; // the number of free i-numbers in the inode array
    int inode[100];
    char flock;
    char ilock;
    char fmod;
    int time[2];
};

// inode is 64 bytes
struct inode{
    unsigned short flags;           //
    char nlinks;                    // number of links to file
    char uid;                       //user ID of owner
    char gid;                       //group ID of owner
    char useless0;                  //ocuppying one byte
    unsigned short useless1;        //
    int size;              // 32 bit size of file
    int addr[11];          //block numbers or device number
    int actime[2];            //time of last access
    int modtime[2];           //time of last modification
};

struct head_of_free{
    int nfree;
    int free[100];
};

void init_inode_blank(struct inode inode_temp){
    inode_temp.flags=0;
    inode_temp.nlinks=0;
    inode_temp.gid = 0;
    inode_temp.useless0=0;
    inode_temp.useless1 =0;
    inode_temp.size = 0;
    for (int i = 0; i<11; i++) {
        inode_temp.addr[i]= 0;
    }
    for (int i = 0; i<2; i++) {
        inode_temp.actime[i]= 0;
        inode_temp.modtime[i]= 0;
    }
}


struct inode get_inode_by_index(int fd_for_file_system,int inode_index){
    struct inode inode_temp;
    
    lseek(fd_for_file_system,1024*2+inode_index*64,SEEK_SET);
    read(fd_for_file_system, &inode_temp, 64);
    return inode_temp;
}


struct superblock get_super_block(int fd_for_file_system){
    struct superblock superblock_temp;
    lseek(fd_for_file_system, 1024, SEEK_SET);
    read(fd_for_file_system, &superblock_temp, sizeof(superblock_temp));
    return superblock_temp;
}

void update_super_block(int fd_for_file_system,
                        struct superblock superblock_temp){
    lseek(fd_for_file_system, 1024, SEEK_SET);
    write(fd_for_file_system, &superblock_temp, sizeof(superblock_temp));
}
void update_inode(int fd_for_file_system, int inode_index, struct inode inode_temp){
    lseek(fd_for_file_system, 2048+inode_index*64, SEEK_SET);
    write(fd_for_file_system, &inode_temp, 64);
}

int get_free_block(int fd_for_file_system){
    struct superblock superblock_temp;
    int block_index; // return value
    struct head_of_free head_of_free;
    superblock_temp = get_super_block(fd_for_file_system);
    int nfree = --superblock_temp.nfree; // nfree decrement
    block_index = superblock_temp.free[nfree];
    if (superblock_temp.nfree==0) {
        lseek(fd_for_file_system, block_index*1024, SEEK_SET);
        read(fd_for_file_system, &head_of_free, sizeof(head_of_free));
        superblock_temp.nfree = 100;
        for (int i=0; i<100; i++) {
            superblock_temp.free[i] = head_of_free.free[i];
        }
    }
    update_super_block(fd_for_file_system, superblock_temp);
    return block_index;
}

unsigned short get_free_inode(int fd_for_file_system, int inodes){
    struct superblock superblock_temp;
    superblock_temp = get_super_block(fd_for_file_system);
    
    int ninode = superblock_temp.ninode;
    if (ninode==0) {
        int j = 0;
        for (int i =1; i<inodes; i++) {
            struct inode inode_temp;
            inode_temp = get_inode_by_index(fd_for_file_system, i);
            int inode_is_allocated = inode_temp.flags>>15;
            if (inode_is_allocated==0&&j<100) {
                superblock_temp.inode[j++] = i;
                superblock_temp.ninode++;
                update_super_block(fd_for_file_system, superblock_temp);
            }
            // if inode[] of superblock is full, break
            if (j==100) {
                break;
            }
        }
        
    }
    ninode = superblock_temp.ninode; // get new superblock ninode.
    unsigned short inode_index = superblock_temp.inode[--ninode];
    superblock_temp.ninode = ninode;
    update_super_block(fd_for_file_system, superblock_temp);
    return inode_index;
}
void init_super_block(int fd_for_file_system, int inode_block){
    struct superblock superblock_temp;
    superblock_temp = get_super_block(fd_for_file_system);
    superblock_temp.isize = inode_block;
    superblock_temp.fsize = inode_block;
    superblock_temp.nfree = 100;
    int dataBlockBegin = 1+1+inode_block;//data block begin position
    for (int i=0; i<100; i++) {
        superblock_temp.free[i] = dataBlockBegin+i;
    }
    superblock_temp.ninode = 100;
    for (int i=0; i<101; i++) {
        superblock_temp.inode[i] = i+1;
    }
    update_super_block(fd_for_file_system, superblock_temp);
}

void init_inodes(int fd_for_file_system, int inodes){
    struct inode inode_temp;
    for (int i=0; i<inodes; i++) {
        inode_temp = get_inode_by_index(fd_for_file_system, i);
        init_inode_blank(inode_temp);
        update_inode(fd_for_file_system, i, inode_temp);
    }
}
void init_first_inode(int fd_for_file_system){
    char parent_file_name[14]="";
    parent_file_name[0]='.';
    parent_file_name[1]='.';
    char child_file_name[14]="";
    child_file_name[0]='.';
    struct inode inode_temp;
    inode_temp = get_inode_by_index(fd_for_file_system, 0);
    inode_temp.flags=52735;
    inode_temp.size = 32;
    int first_block_for_root = get_free_block(fd_for_file_system);
    inode_temp.addr[0]=first_block_for_root;
    lseek(fd_for_file_system, first_block_for_root*1024, SEEK_SET);
    unsigned short root_inode_index = 0;
    write(fd_for_file_system, &root_inode_index, 2);
    write(fd_for_file_system, parent_file_name, 14);
    write(fd_for_file_system, &root_inode_index, 2);
    write(fd_for_file_system, child_file_name, 14);
    update_inode(fd_for_file_system,0, inode_temp);
}
int string_compare(char c1[14], char c2[14]){
    int flag = 1;
    for (int i=0; i<14; i++) {
        if (c1[i]!=c2[i]) {
            flag = 0;
            break;
        }
        if (c1[i]=='\0'&&c2[i]=='\0'&&i>0) {
            return flag;
        }
    }
    return flag;
}
void init_data_block(int fd_for_file_system,
                     int total_block,
                     int inode_block){
    int data_block = total_block-1-1-inode_block;
    int number_of_free_head_block = (data_block-1)/100+1;
    int data_block_begin = 2+ inode_block;
    int initial_nfree = 100;
    for (int i = data_block_begin; i<data_block; i+=100) {
        lseek(fd_for_file_system, i*1024, SEEK_SET);
        write(fd_for_file_system, &initial_nfree, 4);
        for (int j = 0; j<100; j++) {
            int temp = i+100+j;
            write(fd_for_file_system, &temp, 4);
        }
    }
    
}
void initfs(int fd_for_file_system,
            int inode_block,
            int inodes,
            int total_block){
    for (int i=0; i<total_block; i++) {
        char buffer[1024]="";
        lseek(fd_for_file_system, i*1024, SEEK_SET);
        write(fd_for_file_system, buffer, 1024);
    }
    init_super_block(fd_for_file_system,inode_block);
    init_inodes(fd_for_file_system, inodes);
    init_first_inode(fd_for_file_system);
    init_data_block(fd_for_file_system,total_block,inode_block);
}
int map_data_block(int fd_for_file_system,
                   int inode_index,
                   int block_index)
{
    struct inode inode_temp = get_inode_by_index(fd_for_file_system, inode_index);
    if (block_index<10) {
        return inode_temp.addr[block_index];
    }else{
        block_index-=10;
        //third_indirect level
        int block_number_third;
        int index_thrid_level = block_index/256/256/256;
        lseek(fd_for_file_system, inode_temp.addr[10]*1024+index_thrid_level*4, SEEK_SET);//thrid indirect block
        read(fd_for_file_system, &block_number_third, 4);
        //second_indrect level
        int index_second_level = block_index/256/256%256;
        int block_num_second;
        lseek(fd_for_file_system, block_number_third*1024+index_second_level*4, SEEK_SET);
        read(fd_for_file_system, &block_num_second, 4);
        //first-indirect level
        int index_first_level = block_index/256%256;
        int block_num_first;
        lseek(fd_for_file_system, block_num_second*1024+index_first_level*4, SEEK_SET);
        read(fd_for_file_system, &block_num_first, 4);
        //final level
        int index_level = block_index%256;
        int block_num;
        lseek(fd_for_file_system, block_num_first*1024+index_level*4, SEEK_SET);
        read(fd_for_file_system, &block_num, 4);
        return block_num;
    }
}

int my_mkdir_sub (int fd_for_file_system,
                  int inodes,
                  char file_path[],
                  unsigned short parent_inode_index,
                  int offset){
    int return_inode = 0;
    if (offset==strlen(file_path)) {
        return parent_inode_index;
    }
    char parent_file_name[14]="";
    parent_file_name[0]='.';
    parent_file_name[1]='.';
    char child_file_name[14]="";
    child_file_name[0]='.';
    struct inode inode_parent = get_inode_by_index(fd_for_file_system,
                                                   parent_inode_index);
    //flag for exists
    int flag_exits = 0;
    //set up file_name save file name from truncating file path like "/Users/Bob/"
    char file_name[14] = "";
    //j used as index for file_name
    int i=offset , j=0;
    // to get the file_name which may or may not exists
    for (i = offset; i<strlen(file_path)&&file_path[i]!='/'; i++)
    {
        file_name[j++]=file_path[i];
        
    }
    //find total block number
    int block_num = (inode_parent.size-1)/1024+1;
    int block_offset = inode_parent.size%1024; // offset in last data block
    //traverse every block to find out if the file_name exists in the inode
    for (int m = 0; m < block_num ; ++m) {
        int traverse_end = 1024;
        // if reaching the last data block, stop to traverse before offset
        if (m == block_num-1) {
            traverse_end = block_offset;
        }
        // get real_block_index from the method map_data_block
        int real_block_index = map_data_block(fd_for_file_system,
                                              parent_inode_index,m);
        //traverse every entry for this block, every entry is 16 bytes. increment is 16
        for (int k = 0; k < traverse_end; k+=16) {
            // file_name_in_block used as recording the file_name in every entry
            char file_name_in_block[14];
            // offset_lseek used as mark for lseek
            int offset_lseek = real_block_index*1024+k+2;
            // read beginning at this position, 2 is for inode number
            lseek(fd_for_file_system, offset_lseek, SEEK_SET);
            read(fd_for_file_system, file_name_in_block, 14);
            // if file_name is found
            if(string_compare(file_name,file_name_in_block)){
                int offset_lseek = real_block_index*1024+k;
                lseek(fd_for_file_system, offset_lseek, SEEK_SET);
                // find out the inode_index for next searching.
                unsigned short inode_index;
                read(fd_for_file_system, &inode_index, 2);
                //change flag
                flag_exits = 1;
                //change offset
                offset = i;
                return_inode=my_mkdir_sub (fd_for_file_system,
                                           inodes,
                                           file_path,
                                           inode_index,
                                           i+1);
                break;
            }
            //if file_name exists in former block, exiting loop
            if(flag_exits==1) break;
            
        }
        // create directory if file does not exists
        if (flag_exits==0) {
            //get newe inode index
            struct inode inode_temp;
            unsigned short inode_temp_index = get_free_inode(fd_for_file_system, inodes);
            inode_temp = get_inode_by_index(fd_for_file_system, inode_temp_index);
            inode_temp.size = 32;
            inode_temp.flags = 49152; //1 1000 000 000 000 000
            inode_temp.addr[0] = get_free_block(fd_for_file_system);
            update_inode(fd_for_file_system, inode_temp_index, inode_temp);
            lseek(fd_for_file_system, inode_temp.addr[0], SEEK_SET);
            write(fd_for_file_system, &parent_inode_index, 2);
            write(fd_for_file_system, parent_file_name, 14);
            write(fd_for_file_system, &inode_temp_index, 2);
            write(fd_for_file_system, child_file_name, 14);
            //correct the parent inode data block
            inode_parent.size+=16;
            update_inode(fd_for_file_system, parent_inode_index, inode_parent);
            int last_data_block_index = inode_parent.size/1024;
            int last_data_block = map_data_block(fd_for_file_system,
                                                 parent_inode_index, last_data_block_index);
            int last_data_block_write_begin = inode_parent.size%1024-16;
            lseek(fd_for_file_system, last_data_block*1024+last_data_block_write_begin, SEEK_SET);
            write(fd_for_file_system, &inode_temp_index, 2);
            write(fd_for_file_system, file_name, 14);
            return_inode=my_mkdir_sub(fd_for_file_system, inodes, file_path, inode_temp_index, i+1);
        }
    }
    return return_inode;
}

int my_mkdir (int fd_for_file_system,
              int inodes,
              char file_path[])
{
    return my_mkdir_sub(fd_for_file_system,
                        inodes,
                        file_path, 0,1);
    
}
void get_dest_file_name_without_directory(char dest_file_name[],
                                          char dest_file_name_without_directory[14])
{
    int mark = 0;
    int length = (int)strlen(dest_file_name);
    for (int i = length-1 ; dest_file_name[i]!='/'; i--) {
        mark++;
    }
    int j = 0;
    for (int i = length-mark ; i<strlen(dest_file_name); i++) {
        dest_file_name_without_directory[j++] = dest_file_name[i];
    }
}
void get_dest_directory(char dest_file_name[],char dest_directory[100])
{
    int mark = 0;
    int length = (int)strlen(dest_file_name);
    for (int i = length-1 ; dest_file_name[i]!='/'; i--) {
        mark++;
    }
    for (int i = 0 ; i<length-mark; i++) {
        dest_directory[i] = dest_file_name[i];
    }
}

void map_physical_index(int fd_for_file_system, unsigned short inode_index,int index, int physical )
{
    struct inode inode_temp;
    inode_temp = get_inode_by_index(fd_for_file_system, inode_index);
    if (index<10) {
        inode_temp.addr[index] = physical;
        update_inode(fd_for_file_system, inode_index, inode_temp);
    }else{
        index-=10;
        if (inode_temp.addr[10]==0) {
            inode_temp.addr[10] = get_free_block(fd_for_file_system);
            update_inode(fd_for_file_system, inode_index, inode_temp);
        }
        int index_thrid_level = index/256/256/256;
        lseek(fd_for_file_system, inode_temp.addr[10]*1024+index_thrid_level*4, SEEK_SET);
        int block_number_third;
        read(fd_for_file_system, &block_number_third, 4);
        if (block_number_third==0) {
            block_number_third = get_free_block(fd_for_file_system);
            lseek(fd_for_file_system, inode_temp.addr[10]*1024+index_thrid_level*4, SEEK_SET);
            write(fd_for_file_system, &block_number_third, 4);
        }
        int index_second_level =index/256/256%256;
        lseek(fd_for_file_system, block_number_third*1024+index_second_level*4, SEEK_SET);
        int block_num_second;
        read(fd_for_file_system, &block_num_second, 4);
        if (block_num_second==0) {
            block_num_second = get_free_block(fd_for_file_system);
            lseek(fd_for_file_system, block_number_third*1024+index_second_level*4, SEEK_SET);
            write(fd_for_file_system, &block_num_second, 4);
        }
        int index_first_level = index/256%256;
        lseek(fd_for_file_system, block_num_second*1024+index_first_level*4, SEEK_SET);
        int block_num_first;
        read(fd_for_file_system, &block_num_first, 4);
        if (block_num_first==0) {
            block_num_first = get_free_block(fd_for_file_system);
            lseek(fd_for_file_system, block_num_second*1024+index_first_level*4, SEEK_SET);
            write(fd_for_file_system, &block_num_first, 4);
        }
        int index_level = index%256;
        lseek(fd_for_file_system, block_num_first*1024+index_level*4, SEEK_SET);
        write(fd_for_file_system, &physical, 4);
        
    }
}
int get_parent_index(int fd_for_file_system,
                     char dest_file_name[],
                     int inodes)
{
    char dest_file_name_without_directory[14]="";
    char dest_directory[100] = "";
    get_dest_file_name_without_directory(dest_file_name,dest_file_name_without_directory);
    get_dest_directory(dest_file_name,dest_directory);
    return my_mkdir(fd_for_file_system, inodes, dest_directory);
}
void cpin(int fd_for_file_system,
          int fd_for_copy_in,
          char dest_file_name[],
          int inodes)
{
    
    char dest_file_name_without_directory[14]="";
    get_dest_file_name_without_directory(dest_file_name,dest_file_name_without_directory);
    int parent_inode_index = get_parent_index(fd_for_file_system,
                                              dest_file_name,
                                              inodes);
    //create file inode
    unsigned short file_inode_index = get_free_inode(fd_for_file_system, inodes);
    // write the end of parent inode
    struct inode parent_inode = get_inode_by_index(fd_for_file_system, parent_inode_index);
    int last_parent_inode_block_index = parent_inode.size/1024;
    int physical_last_parent = map_data_block(fd_for_file_system, parent_inode_index, last_parent_inode_block_index);
    int offset_in_last_block = parent_inode.size%1024;
    lseek(fd_for_file_system, physical_last_parent*1024+offset_in_last_block, SEEK_SET);
    write(fd_for_file_system, &file_inode_index, 2);
    write(fd_for_file_system, dest_file_name_without_directory, 14);
    parent_inode.size+=16;
    update_inode(fd_for_file_system, parent_inode_index, parent_inode);
    //copy file content
    parent_inode = get_inode_by_index(fd_for_file_system, parent_inode_index);
    struct inode inode_file = get_inode_by_index(fd_for_file_system, file_inode_index);
    inode_file.flags = 32768; // 1,000,000,000,000,000
    update_inode(fd_for_file_system, file_inode_index, inode_file);
    lseek(fd_for_copy_in, 0, SEEK_SET);
    char buffer[1024] = "";
    int index_in_inode_block = 0;
    int offset =0;
    while ((offset = (int)read(fd_for_copy_in, buffer, 1024))!=0) {
        int physical_block = get_free_block(fd_for_file_system);
        lseek(fd_for_file_system, physical_block*1024, SEEK_SET);
        write(fd_for_file_system, buffer, offset);
        map_physical_index(fd_for_file_system,file_inode_index, index_in_inode_block, physical_block);
        index_in_inode_block++;
        inode_file = get_inode_by_index(fd_for_file_system, file_inode_index);
        inode_file.size+=offset;
        update_inode(fd_for_file_system, file_inode_index, inode_file);
        //for (int i=0; i<1024; i++)  buffer[i]='\0';
    }
}
int find_file_index(int fd_for_file_system,
                    char file_name[],
                    int inodes){
    char dest_file_name_without_directory[14]="";
    char dest_directory[100] = "";
    get_dest_file_name_without_directory(file_name,dest_file_name_without_directory);
    get_dest_directory(file_name,dest_directory);
    
    int parent_inode_index = my_mkdir(fd_for_file_system, inodes, dest_directory);
    unsigned short inode_index = 0;
    struct inode parent_inode = get_inode_by_index(fd_for_file_system, parent_inode_index);
    
    //find total block number
    int block_num = (parent_inode.size-1)/1024+1;
    int block_offset = parent_inode.size%1024; // offset in last data block
    //traverse every block to find out if the file_name exists in the inode
    for (int m = 0; m < block_num ; ++m) {
        int traverse_end = 1024;
        // if reaching the last data block, stop to traverse before offset
        if (m == block_num-1) {
            traverse_end = block_offset;
        }
        // get real_block_index from the method map_data_block
        int real_block_index = map_data_block(fd_for_file_system,
                                              parent_inode_index,m);
        //traverse every entry for this block, every entry is 16 bytes. increment is 16
        for (int k = 0; k < traverse_end; k+=16) {
            // file_name_in_block used as recording the file_name in every entry
            char file_name_in_block[14];
            // offset_lseek used as mark for lseek
            int offset_lseek = real_block_index*1024+k+2;
            // read beginning at this position, 2 is for inode number
            lseek(fd_for_file_system, offset_lseek, SEEK_SET);
            read(fd_for_file_system, file_name_in_block, 14);
            // if file_name is found
            if(string_compare(dest_file_name_without_directory,file_name_in_block)){
                int offset_lseek = real_block_index*1024+k;
                lseek(fd_for_file_system, offset_lseek, SEEK_SET);
                // find out the inode_index for source file
                read(fd_for_file_system, &inode_index, 2);
            }
        }
    }
    return inode_index;
    
}
void cpout(int fd_for_file_system,
           int fd_for_copy_out,
           char file_name[],
           int inodes)
{
    int inode_index = find_file_index(fd_for_file_system,
                                      file_name,
                                      inodes);
    //get source file inode by inode_index
    struct inode file_inode = get_inode_by_index(fd_for_file_system, inode_index);
    //calculate how many blocks file has
    int block_num = (file_inode.size-1)/1024+1;
    
    lseek(fd_for_copy_out, 0, SEEK_SET);
    char buffer[1024]="";
    for (int i=0; i<block_num; i++) {
        if (i==block_num-1) {
            int physical = map_data_block(fd_for_file_system, inode_index, i);
            int offset = file_inode.size%1024;
            lseek(fd_for_file_system, physical*1024, SEEK_SET);
            read(fd_for_file_system, buffer, offset);
            write(fd_for_copy_out, buffer, offset);
            continue;
        }
        int physical = map_data_block(fd_for_file_system, inode_index, i);
        lseek(fd_for_file_system, physical*1024, SEEK_SET);
        int offset = (int)read(fd_for_file_system, buffer, 1024);
        write(fd_for_copy_out, buffer, offset);
    }
    
}
void add_free_block(int fd_for_file_system, int physic)
{
    struct superblock superblock_temp = get_super_block(fd_for_file_system);
    if (superblock_temp.nfree==100) {
        lseek(fd_for_file_system, physic*1024, SEEK_SET);
        write(fd_for_file_system, &superblock_temp.nfree, 4);
        write(fd_for_file_system, superblock_temp.free, 400);
        superblock_temp.nfree = 1;
        superblock_temp.free[0] = physic;
        update_super_block(fd_for_file_system, superblock_temp);
    }else{
        superblock_temp.nfree++;
        int tem = superblock_temp.nfree;
        superblock_temp.free[tem-1] = physic;
        update_super_block(fd_for_file_system, superblock_temp);
    }
}
void rm(int fd_for_file_system, char file_name[], int inodes){
    int inode_index_parent = get_parent_index(fd_for_file_system, file_name, inodes);
    struct inode inode_parent = get_inode_by_index(fd_for_file_system, inode_index_parent);
    int inode_index = find_file_index(fd_for_file_system,
                                      file_name,
                                      inodes);
    char dest_file_name_without_directory[14];
    get_dest_file_name_without_directory(file_name, dest_file_name_without_directory);
    struct inode inode_temp = get_inode_by_index(fd_for_file_system, inode_index);
    int block_num  = (inode_temp.size-1)/1024+1;
    for (int i=0; i<block_num; i++) {
        char buffer[1024] ="";
        int physical = map_data_block(fd_for_file_system, inode_index, i);
        lseek(fd_for_file_system, physical*1024, SEEK_SET);
        write(fd_for_file_system, buffer, 1024);
        add_free_block(fd_for_file_system, physical);
    }
    init_inode_blank(inode_temp);
    update_inode(fd_for_file_system, inode_index, inode_temp);
    
    int parent_block_num = (inode_parent.size-1)/1024+1;
    int block_offset = inode_parent.size%1024; // offset in last data block
    //traverse every block to find out if the file_name exists in the inode
    for (int m = 0; m < parent_block_num ; ++m) {
        int traverse_end = 1024;
        // if reaching the last data block, stop to traverse before offset
        if (m == parent_block_num-1) {
            traverse_end = block_offset;
        }
        // get real_block_index from the method map_data_block
        int real_block_index = map_data_block(fd_for_file_system,
                                              inode_index_parent,m);
        //traverse every entry for this block, every entry is 16 bytes. increment is 16
        int flag = 0;
        for (int k = 0; k < traverse_end; k+=16) {
            // file_name_in_block used as recording the file_name in every entry
            char file_name_in_block[14];
            // offset_lseek used as mark for lseek
            int offset_lseek = real_block_index*1024+k+2;
            // read beginning at this position, 2 is for inode number
            lseek(fd_for_file_system, offset_lseek, SEEK_SET);
            read(fd_for_file_system, file_name_in_block, 14);
            // if file_name is found
            if(string_compare(dest_file_name_without_directory,file_name_in_block)){
                int offset_lseek = real_block_index*1024+k;
                lseek(fd_for_file_system, offset_lseek, SEEK_SET);
                char buffer[16] = "";
                write(fd_for_file_system, buffer, 16);
                flag = 1;
                break;
            }
            
        }
        if (flag==1) {
            break;
        }
    }
}
void show_directory_structure(int fd_for_file_system, int inodes){
    struct inode inode_root = get_inode_by_index(fd_for_file_system, 0);
    for (int i = 0; i<inode_root.size; i+=16) {
        lseek(fd_for_file_system, inode_root.addr[0]*1024+i+2, SEEK_SET);
        char file_name[14] = "";
        read(fd_for_file_system, file_name, 14);
        printf(file_name);
        printf("\n");
    }
}
int main(){
    char initfs_name[] = "initfs";
    char q_name[]="q";
    char mkdir_name[]="mkdir";
    char cpin_name[]="cpin";
    char cpout_name[]="cpout";
    char rm_name[]="rm";
    char show_root[]="showRoot";
    char show_free[]="showFree";
    //users'input parameters
    char method_name[100];
    char file_path[100];
    //file system global variable
    int total_block=0;
    int inodes=0;
    int fd_for_file_system = 0;
    int inode_block=0;
    
    while (1) {
        printf("Please enter method name: \n");
        scanf("%s", method_name);
        //gets(methodName);
        if (strcmp(method_name, initfs_name) == 0){
            printf("Please enter file path: \n");
            scanf("%s", file_path);
            //gets(filePath);
            printf("Please enter total number of blocks: \n");
            scanf("%d",&total_block);
            printf("Please enter total number of inodes: \n");
            scanf("%d", &inodes);
            inode_block = (inodes-1)/16+1;
            fd_for_file_system = open(file_path, 2);
            initfs(fd_for_file_system,inode_block, inodes, total_block);
        }else if (strcmp(method_name, q_name)==0) {
                printf("quit");
                break;
        }
        else if(strcmp(method_name, cpin_name)==0){
            printf("Please source file path: \n");
            char out_source_file[100];
            scanf("%s", out_source_file);
            int fd_for_copy_in = open(out_source_file, 2);
            printf("Please destination file path: \n");
            char in_destination_file[100];
            scanf("%s", in_destination_file);
            cpin(fd_for_file_system, fd_for_copy_in, in_destination_file, inodes);
        }else if(strcmp(method_name, cpout_name)==0){
            printf("Please source file path: \n");
            char in_source_file[100];
            scanf("%s", in_source_file);
            printf("Please destination file path: \n");
            char out_destination_file[100];
            scanf("%s", out_destination_file);
            int fd_for_copy_out = open(out_destination_file, 2);
            cpout(fd_for_file_system, fd_for_copy_out, in_source_file, inodes);
        }else if(strcmp(method_name, rm_name)==0){
            printf("Please file path: \n");
            char in_source_file[100];
            scanf("%s", in_source_file);
            rm(fd_for_file_system, in_source_file, inodes);
        }else if(strcmp(method_name, mkdir_name)==0){
            printf("Please file path end with '/': \n");
            char in_source_file[100];
            scanf("%s", in_source_file);
            my_mkdir(fd_for_file_system, inodes, in_source_file);
        }else if(strcmp(method_name, show_root)==0){
            show_directory_structure(fd_for_file_system,inodes);
        }else if(strcmp(method_name, show_free)==0){
            lseek(fd_for_file_system, 21*1024, SEEK_SET);
            struct superblock superblock_temp = get_super_block(fd_for_file_system);
            printf("nfree for now is %d", superblock_temp.nfree);
            printf("\n");
            for (int i=0; i<superblock_temp.nfree; i++) {
                printf("%d", superblock_temp.free[i]);
                printf("\n");
            }
        }
    }
    return 0;
}




