// student id: 2023202276
// please change the above line to your student id
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
void printSummary(int hits, int misses, int evictions) {
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
    FILE* output_fp = fopen(".csim_results", "w");
    assert(output_fp);
    fprintf(output_fp, "%d %d %d\n", hits, misses, evictions);
    fclose(output_fp);
}
void printHelp(const char* name) {
    printf(
        "Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n"
        "Options:\n"
        "  -h         Print this help message.\n"
        "  -v         Optional verbose flag.\n"
        "  -s <num>   Number of set index bits.\n"
        "  -E <num>   Number of lines per set.\n"
        "  -b <num>   Number of block offset bits.\n"
        "  -t <file>  Trace file.\n\n"
        "Examples:\n"
        "  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n"
        "  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n",
        name, name, name);
}
typedef struct cache_line {
    int valid;
    int tag;
    int time;
} Cache_line;
typedef struct cache_ {
    int S;
    int E;
    int B;
    Cache_line** line;
} Cache;
int hit_count = 0, miss_count = 0, eviction_count = 0;
int verbose = 0;
char t[10000];
Cache* cache = NULL;
void Freecache() {
    for (int i = 0; i < cache->S; i++) {
        free(cache->line[i]);
    }
    free(cache->line);
    free(cache);
}

int find_line_or_empty(int op_s, int op_tag) {
    for (int i = 0; i < cache->E; i++) {
        if (cache->line[op_s][i].valid && cache->line[op_s][i].tag == op_tag) {
            return i;
        }
        if (!cache->line[op_s][i].valid) {
            return i;
        }
    }
    return -1;
}
int find_lru(int op_s) {
    int lru_index = 0;
    for (int i = 1; i < cache->E; i++) {
        if (cache->line[op_s][i].time > cache->line[op_s][lru_index].time) {
            lru_index = i;
        }
    }
    return lru_index;
}
void Initcache(int s, int E, int b) {
    int S = 1 << s;
    cache = (Cache*)malloc(sizeof(Cache));
    cache->S = S;
    cache->E = E;
    cache->B = 1 << b;
    cache->line = (Cache_line**)malloc(sizeof(Cache_line*) * S);
    for (int i = 0; i < S; i++) {
        cache->line[i] = (Cache_line*)malloc(sizeof(Cache_line) * E);
        for (int j = 0; j < E; j++) {
            cache->line[i][j].valid = 0;
            cache->line[i][j].tag = -1;
            cache->line[i][j].time = 0;
        }
    }
}

void update_cache(int op_s, int line_index, int op_tag) {
    cache->line[op_s][line_index].valid = 1;
    cache->line[op_s][line_index].tag = op_tag;
    cache->line[op_s][line_index].time = 0;
    for (int i = 0; i < cache->E; i++) {
        if (cache->line[op_s][i].valid) {
            cache->line[op_s][i].time++;
        }
    }
}
void handle_cache_access(int op_s, int op_tag) {
    int line_index = find_line_or_empty(op_s, op_tag);
    if (line_index >= 0 && cache->line[op_s][line_index].valid) {
        hit_count++;
        if (verbose) printf("hit ");
    } else {
        miss_count++;
        if (verbose) printf("miss ");
        if (line_index == -1) {
            eviction_count++;
            if (verbose) printf("eviction ");
            line_index = find_lru(op_s);
        }
    }
    update_cache(op_s, line_index, op_tag);
}
void get_Trace(int s, int E, int b) {
    FILE* pFile = fopen(t, "r");
    char identifier;
    unsigned address;
    int size;
    while (fscanf(pFile, " %c %x,%d", &identifier, &address, &size) > 0) {
        int op_tag = address >> (s + b);
        int op_s = (address >> b) & ((1 << s) - 1);
        if (identifier == 'L' || identifier == 'S') {
            handle_cache_access(op_s, op_tag);
        }
    }
    fclose(pFile);
}
int main(int argc, char* argv[]) {
    char opt;
    int s = 0, E = 0, b = 0;
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
            case 'h':
                printHelp(argv[0]);
                exit(0);
            case 'v':
                verbose = 1;
                break;
            case 't':
                strcpy(t, optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 's':
                s = atoi(optarg);
                break;
            default:
                printHelp(argv[0]);
                exit(-1);
        }
    }
    Initcache(s, E, b);
    get_Trace(s, E, b);
    Freecache();
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
