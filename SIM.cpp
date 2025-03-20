#include <iostream>
#include <queue>
#include <array>
#include <cstring>
#include <cmath>
#include <fstream>
#include <bitset>

#define WRITE_THROUGH 0
#define LRU 0
#define EMPTY -1
#define WRITE_BACK 1
#define FIFO 1

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
                this->read(std::stoll(str.substr(4), nullptr, 16));
            } else {
                this->write(std::stoll(str.substr(4), nullptr, 16));
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
        return value>>((int)log2(this->set_size) + 6);


    }

    inline long long offset(const long long value){
        return value & 0b111111; // ALWAYS 6 bits (log2(64) = 6)
    }

    inline long long index(const long long value){
        return (value ^ tag(value) << ((int)log2(this->set_size) + 6) ^ offset(value)) >> 6;
    }


    void read(const long long mem){ //reads from cache
        //std::cout << index(mem) << '\n';
        long long mem_index = index(mem); // the index of memory
        long long mem_tag = tag(mem);
        for (int i = 0; i < this->assoc; i++)
        {
            if(this->cache_arr[mem_index][i].tag == mem_tag){ //  A read hit 
                if(this->replacement == LRU){ // if lru, move up
                    int8_t dirty_temp = this->cache_arr[mem_index][i].dirty;
                    while(i < this->assoc-1 && this->cache_arr[mem_index][i+1].dirty != -1){
                        this->cache_arr[mem_index][i].tag = this->cache_arr[mem_index][i+1].tag;
                        this->cache_arr[mem_index][i].dirty = this->cache_arr[mem_index][i+1].dirty;
                        i++;
                    }
                    this->cache_arr[mem_index][i].tag = mem_tag;
                    this->cache_arr[mem_index][i].dirty = dirty_temp;
                }
                this->hit_ctr++;

                return;
            }
        }
        this->mem_reads++;
        this->miss_ctr++;

    }

    void write(const long long mem){
        //std::cout<< is_dirty(index(mem)) << '\n';
        long long mem_index = index(mem); // the index of memory
        long long mem_tag = tag(mem);
        
        for (int i = 0; i < this->assoc; i++) // iterate through the queue 
        {
            if(this->cache_arr[mem_index][i].dirty == EMPTY){ // cache write MISS and queue is NOT empty
                this->cache_arr[mem_index][i].tag = mem_tag;
                this->cache_arr[mem_index][i].dirty = 0; // new block, no dirtyy bit 
                if(this->wb == WRITE_THROUGH) this->mem_writes++;
                this->miss_ctr++;
                return;
            }
            if(this->cache_arr[mem_index][i].tag == mem_tag){ // cache write HIT
                if(this->replacement == LRU){
                    while(i < this->assoc-1 && this->cache_arr[mem_index][i+1].dirty != -1){
                        this->cache_arr[mem_index][i].tag = this->cache_arr[mem_index][i+1].tag;
                        this->cache_arr[mem_index][i].dirty = this->cache_arr[mem_index][i+1].dirty;
                        i++;
                    }
                    this->cache_arr[mem_index][i].tag = mem_tag;

                }

                if(this->wb == WRITE_THROUGH){ // copy contents in memory too
                    this->mem_writes++;
                } else { // if write back, mark mem tag as dirty
                    this->cache_arr[mem_index][i].dirty = 1;
                }

                this->hit_ctr++;
                return;
            }
        }
        
        // cache is FULL ( need replacement policy)
        // since FIFO and LRU order is maintained, simply shift all elements and assign the top to the new

        // if dirty to-be-evicted bit is on
        // only ever on if write-back 
        if (this->cache_arr[mem_index][0].dirty == 1){
            this->mem_writes++;
        }

        int i =0;
        while(i<this->assoc-1){
            this->cache_arr[mem_index][i].tag = this->cache_arr[mem_index][i+1].tag;
            this->cache_arr[mem_index][i].dirty = this->cache_arr[mem_index][i+1].dirty;
            i++;
        }
        this->cache_arr[mem_index][i].tag = mem_tag;
        this->cache_arr[mem_index][i].dirty = false;
        
        this->miss_ctr++;
        

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
        "\nset size:\t" << this->set_size << 
        "\nblock size:\t64" << 
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
    return 0;   
}