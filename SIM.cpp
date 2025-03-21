#include <iostream>
#include <queue>
#include <array>
#include <cstring>
#include <cmath>
#include <fstream>
#include <bitset>

#define EMPTY -1
#define READ (rw) // READ/WRITE 1 = READ, 0 = WRITE
#define WRITE (!(rw))
#define WRITE_BACK (this->wb)
#define WRITE_THROUGH (!(this->wb))
#define FIFO (this->replacement)
#define LRU (!(this->replacement))

typedef struct block{
    int8_t dirty; // -1: empty, 0: not dirty, 1: dirty
    long long tag;
}block;

class cache{
    long long int cache_size; // the cache size
    int assoc; // associativity
    int set_size; // the amount of pointers of the cache arr
    int mem_writes;
    int mem_reads;
    int miss_ctr;
    int hit_ctr;
    bool replacement;   // 0: LRU, 1: FIFO
    bool wb;        // 0: write-through, 1: write-back

    block** cache_arr; // the actual cache
    


    public:
    cache(char* argv[]):
        cache_size((long long int)strtoull(argv[1], (char**)NULL, 10)),
        assoc((int)strtoul(argv[2], (char**)NULL,10)),
        set_size(this->cache_size/(64*this->assoc)),
        mem_writes(0),
        mem_reads(0),
        miss_ctr(0),
        hit_ctr(0),
        replacement(*(argv[3])-'0'),
        wb(*(argv[4])-'0'),
        cache_arr((block**)malloc(this->set_size*sizeof(block*)))
    {

        for (int i = 0; i < this->set_size; i++) // allocates memory 
        {
            this->cache_arr[i] = (block*)calloc(this->assoc, 64); // each cache has n associativity blocks, where each block is 64 bytes
            for (int j = 0; j < this->assoc; j++)
            {
                this->cache_arr[i][j].dirty = -1;
            }
            
            
        }

        std::string str; // the line 
        std::ifstream file(argv[5]);

        while(getline(file,str)){
            if(str[0] == 'R'){
                this->read_write(std::stoll(str.substr(4), nullptr, 16), true); // 1 = read
            } else if (str[0] == 'W') {
                this->read_write(std::stoll(str.substr(4), nullptr, 16), false); // 0 = write
            }
        }
        
    };

    ~cache(){
        for(int i=0; i< this->set_size; i++){
            free(this->cache_arr[i]);
        }

        free(this->cache_arr); 


    };

    inline long long tag(const long long value){
        return value >> (6+(int)log2(this->set_size));

    }

    inline long long index(const long long value){
        return (value >> 6) & (1<<((int)log2(this->set_size)))-1;
    }

    inline long long offset(const long long value){
        return value & ((1<<6)-1);
    }

    // insert
    void insert(long long mem_index, long long mem_tag, int i, const bool rw){
        if(READ){ // if the cache miss is a READ
            this->mem_reads++; // reads from memory
        } else if(WRITE_THROUGH) { // if a write (cache miss is NOT a read and policy is write through)
            this->mem_writes++; // write to memory instantly
        }
        this->cache_arr[mem_index][i].tag = mem_tag; 
        this->cache_arr[mem_index][i].dirty = 0;
        this->miss_ctr++; // increase the miss ctr
    }

    void read_write(const long long mem, const bool rw){ // read or write 

        //std::cout<< is_dirty(index(mem)) << '\n';
        long long mem_index = index(mem); // the index of memory
        long long mem_tag = tag(mem);
        
        for (int i = 0; i < this->assoc; i++) // iterate through the queue 
        {
            if(this->cache_arr[mem_index][i].dirty == EMPTY){ // cache MISS (curr block is empty)
                insert(mem_index, mem_tag, i, rw);
                return;
                
            }
            if(this->cache_arr[mem_index][i].tag == mem_tag){ // cache  HIT
                

                // i is now at current element of last (if lru)

                if(WRITE){
                    if(WRITE_THROUGH){ // writes to memory as well (in a cache write hit )
                        this->mem_writes++; // writes to memory
                    } else { // if write back
                        this->cache_arr[mem_index][i].dirty = 1; // block is now DIRTY
                    }
                }


                if(LRU){ // if lru, move to front
                    block temp = this->cache_arr[mem_index][i];
                    int j = i;
                    while(j < this->assoc-1 &&this->cache_arr[mem_index][j+1].dirty != EMPTY ){
                        this->cache_arr[mem_index][j].tag = this->cache_arr[mem_index][j+1].tag;
                        this->cache_arr[mem_index][j].dirty = this->cache_arr[mem_index][j+1].dirty;
                        j++;
                    }
                    this->cache_arr[mem_index][j] = temp;
                    
                }

                this->hit_ctr++;
                return;
            }
        }
        
        // cache miss AND FULL ( need replacement policy) (a miss)


        // if write back and the to-be-evicted block is DIRTY
        if(WRITE_BACK && this->cache_arr[mem_index][0].dirty == 1){
            this->mem_writes++;
        }

        int j = 0;
        while(j < this->assoc-1){
            this->cache_arr[mem_index][j].tag = this->cache_arr[mem_index][j+1].tag;
            this->cache_arr[mem_index][j].dirty = this->cache_arr[mem_index][j+1].dirty;
            j++;
        }

        
        insert(mem_index, mem_tag, j, rw);

    }


    void debug(){ // debug 
        for (int i = 0; i < this->set_size; i++)
        {   
            printf("set #%i:\t bottom <\t",i);
            for (int j = 0; j < this->assoc; j++)
            {
                printf("%i, %llx\t", this->cache_arr[i][j].dirty, this->cache_arr[i][j].tag);
            }
            printf("> top\n");
            
        }

        std::cout << 
        "\ncache size:\t" << this->cache_size <<  
        "\nnum of sets:\t" << this->set_size << 
        "\ncache blocks per set:\t64" << 
        "\nassociativity:\t" << this->assoc << 
        "\nfifo?:\t\t" << this->replacement << 
        "\nwrite-back?:\t" << this->wb <<
        "\nmem writes:\t" << this->mem_writes <<
        "\nmem reads:\t" << this->mem_reads <<
        "\nmiss counter:\t" << this->miss_ctr <<
        "\nhit counter\t" << this->hit_ctr <<
        "\nmiss ratio:\t" << ((double)this->miss_ctr/(double)(this->miss_ctr+this->hit_ctr)) << '\n';
    }

    void print(){
        std::cout << 
        "Miss ratio " << ((double)this->miss_ctr/(double)(this->miss_ctr+this->hit_ctr))<<
        "\nwrite " << this->mem_writes <<
        "\nread " << this->mem_reads << '\n';
    }
};

int main(int argc, char* argv[]){
    cache a(argv);
    a.debug();
    a.print();
    // std::cout << std::bitset<32>(a.tag(0x123456789)) << '\n' << 
    // std::bitset<32>(a.index(0x123456789)) << '\n' << 
    // std::bitset<32>(a.offset(0x123456789)) << '\n';
    return 0;   
}