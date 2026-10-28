#include<bits/stdc++.h>
using namespace std;

class FreeListAllocator{

    struct Header{
        long long int size;     // payload ka size
        bool isFree;
        Header* prev;           // address order mein pichhla block
    };

    // sirf free block ke payload ke andar rehta hai
    struct FreeLinks{
        Header* fprev;
        Header* fnext;
    };

    char* pool;
    char* endPtr;
    Header* freeHead;           // sirf free blocks ki list
    long long int poolSize;
    long long int H;            // header ka size
    long long int MINP;         // minimum payload

    long long int liveBlocks;
    long long int liveBytes;

    FreeLinks* links(Header* h){
        return reinterpret_cast<FreeLinks*>(reinterpret_cast<char*>(h)+H);
    }

    // agla block = current + H + size
    Header* nextOf(Header* h){

        char* p=reinterpret_cast<char*>(h)+H+h->size;

        if(p>=endPtr){
            return nullptr;
        }

        return reinterpret_cast<Header*>(p);
    }

    void addFree(Header* h){

        FreeLinks* l=links(h);

        l->fprev=nullptr;
        l->fnext=freeHead;

        if(freeHead!=nullptr){
            links(freeHead)->fprev=h;
        }

        freeHead=h;
        h->isFree=true;
    }

    void removeFree(Header* h){

        FreeLinks* l=links(h);

        if(l->fprev!=nullptr){
            links(l->fprev)->fnext=l->fnext;
        }

        else{
            freeHead=l->fnext;
        }

        if(l->fnext!=nullptr){
            links(l->fnext)->fprev=l->fprev;
        }

        h->isFree=false;
    }

public:

    FreeListAllocator(long long int total){

        H=sizeof(Header);
        MINP=sizeof(FreeLinks);

        poolSize=total&~7LL;
        pool=new char[poolSize];
        endPtr=pool+poolSize;

        liveBlocks=0;
        liveBytes=0;
        freeHead=nullptr;

        Header* first=reinterpret_cast<Header*>(pool);
        first->size=poolSize-H;
        first->prev=nullptr;

        addFree(first);
    }

    ~FreeListAllocator(){
        leakReport();
        delete[] pool;
    }

    FreeListAllocator(const FreeListAllocator&)=delete;
    FreeListAllocator& operator=(const FreeListAllocator&)=delete;

    void* allocate(long long int n){

        if(n<=0 || n>poolSize){
            return nullptr;
        }

        n=(n+7)&~7LL;

        if(n<MINP){
            n=MINP;
        }

        Header* cur=freeHead;

        while(cur!=nullptr){

            if(cur->size>=n){

                removeFree(cur);

                if(cur->size>=n+H+MINP){

                    Header* newBlock=reinterpret_cast<Header*>(reinterpret_cast<char*>(cur)+H+n);

                    newBlock->size=cur->size-n-H;
                    newBlock->prev=cur;

                    cur->size=n;

                    Header* after=nextOf(newBlock);

                    if(after!=nullptr){
                        after->prev=newBlock;
                    }

                    addFree(newBlock);
                }

                liveBlocks++;
                liveBytes+=cur->size;

                return reinterpret_cast<char*>(cur)+H;
            }

            cur=links(cur)->fnext;
        }

        return nullptr;
    }

    void deallocate(void* p){

        if(p==nullptr){
            return;
        }

        char* cp=static_cast<char*>(p);

        if(cp<pool+H || cp>=endPtr || (cp-pool)%8!=0){
            cout<<"ERROR: pointer is pool ka nahi hai!"<<endl;
            return;
        }

        Header* h=reinterpret_cast<Header*>(cp-H);

        if(h->isFree){
            cout<<"ERROR: double free detect hua!"<<endl;
            return;
        }

        liveBlocks--;
        liveBytes-=h->size;

        // right neighbour free hai to merge
        Header* nx=nextOf(h);

        if(nx!=nullptr && nx->isFree){

            removeFree(nx);
            h->size+=H+nx->size;

            Header* after=nextOf(h);

            if(after!=nullptr){
                after->prev=h;
            }
        }

        // left neighbour free hai to merge
        Header* pv=h->prev;

        if(pv!=nullptr && pv->isFree){

            removeFree(pv);
            pv->size+=H+h->size;

            Header* after=nextOf(pv);

            if(after!=nullptr){
                after->prev=pv;
            }

            h=pv;
        }

        addFree(h);
    }

    void leakReport(){

        if(liveBlocks==0){
            cout<<"[leak check] koi leak nahi, sab free hai"<<endl;
        }

        else{
            cout<<"[leak check] LEAK: "<<liveBlocks<<" block(s), "<<liveBytes<<" bytes free nahi hue"<<endl;
        }
    }

    void printBlocks(){

        Header* cur=reinterpret_cast<Header*>(pool);
        int idx=0;

        while(cur!=nullptr){

            long long int offset=reinterpret_cast<char*>(cur)-pool;

            cout<<"block "<<idx<<" | offset="<<offset<<" | size="<<cur->size<<" | "<<(cur->isFree?"FREE":"USED")<<endl;

            cur=nextOf(cur);
            idx++;
        }

        cout<<"------"<<endl;
    }
};

void benchmark(int N){

    const int ROUNDS=2000;

    vector<void*> ptrs(N);

    cout<<"N = "<<N<<endl;

    // apna allocator
    {
        FreeListAllocator alloc(1<<22);

        unsigned long long checksum=0;

        auto start=chrono::high_resolution_clock::now();

        for(int r=0; r<ROUNDS; r++){

            for(int i=0; i<N; i++){
                ptrs[i]=alloc.allocate(32);
                checksum+=reinterpret_cast<unsigned long long>(ptrs[i]);
            }

            for(int i=0; i<N; i++){
                alloc.deallocate(ptrs[i]);
            }
        }

        auto end=chrono::high_resolution_clock::now();

        cout<<"apna allocator : "<<chrono::duration<double,milli>(end-start).count()<<" ms"<<endl;
        cout<<"checksum: "<<checksum<<endl;
    }

    // new/delete
    {
        unsigned long long checksum=0;

        auto start=chrono::high_resolution_clock::now();

        for(int r=0; r<ROUNDS; r++){

            for(int i=0; i<N; i++){
                ptrs[i]=new char[32];
                checksum+=reinterpret_cast<unsigned long long>(ptrs[i]);
            }

            for(int i=0; i<N; i++){
                delete[] static_cast<char*>(ptrs[i]);
            }
        }

        auto end=chrono::high_resolution_clock::now();

        cout<<"new/delete     : "<<chrono::duration<double,milli>(end-start).count()<<" ms"<<endl;
        cout<<"checksum: "<<checksum<<endl;
    }

    cout<<endl;
}

int main(){

    cout<<"=== Coalescing demo ==="<<endl;
    {
        FreeListAllocator alloc(256);

        void* a=alloc.allocate(20);
        void* b=alloc.allocate(50);
        void* c=alloc.allocate(10);
        (void)c;

        alloc.printBlocks();

        alloc.deallocate(a);
        alloc.deallocate(b);
        cout<<"a aur b free karne ke baad (merge hona chahiye):"<<endl;
        alloc.printBlocks();

        void* d=alloc.allocate(80);
        cout<<"allocate(80) = "<<d<<" (ab nullptr nahi aana chahiye)"<<endl;

        cout<<"double free test:"<<endl;
        alloc.deallocate(d);
        alloc.deallocate(d);

        // c ko jaanbujhke free nahi kiya, destructor leak batayega
    }

    cout<<endl<<"=== Benchmark ==="<<endl;
    benchmark(1000);
    benchmark(2000);
    benchmark(4000);

    return 0;
}