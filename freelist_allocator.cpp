#include<bits/stdc++.h>
using namespace std;

class FreeListAllocator{

    struct Header{
        long long int size;     // sirf payload ka size (header chhod ke)
        bool isFree;
        Header* next;           // address order mein agla block
    };

    char* pool;
    Header* head;
    long long int poolSize;

public:

    FreeListAllocator(long long int total){

        poolSize=total;
        pool=new char[poolSize];

        long long int H=sizeof(Header);

        // shuru mein poora pool ek bada free block hai
        head=reinterpret_cast<Header*>(pool);
        head->size=poolSize-H;
        head->isFree=true;
        head->next=nullptr;
    }

    ~FreeListAllocator(){
        delete[] pool;
    }

    FreeListAllocator(const FreeListAllocator&)=delete;
    FreeListAllocator& operator=(const FreeListAllocator&)=delete;

    void* allocate(long long int n){

        if(n<=0){
            return nullptr;
        }

        // 8 ke multiple tak round up
        n=(n+7)&~7;

        long long int H=sizeof(Header);

        Header* cur=head;

        // first fit: pehla free block jo kaafi bada ho
        while(cur!=nullptr){

            if(cur->isFree && cur->size>=n){

                // itna bacha hai ki naya block ban sake? to split karo
                if(cur->size>=n+H+8){

                    Header* newBlock=reinterpret_cast<Header*>(reinterpret_cast<char*>(cur)+H+n);

                    newBlock->size=cur->size-n-H;
                    newBlock->isFree=true;
                    newBlock->next=cur->next;

                    cur->size=n;
                    cur->next=newBlock;
                }

                cur->isFree=false;

                // user ko payload ka address do, header ka nahi
                return reinterpret_cast<char*>(cur)+H;
            }

            cur=cur->next;
        }

        return nullptr;     // koi block kaafi bada nahi mila
    }

    void deallocate(void* p){

        if(p==nullptr){
            return;
        }

        long long int H=sizeof(Header);

        // payload se H bytes peeche = header
        Header* h=reinterpret_cast<Header*>(static_cast<char*>(p)-H);
        h->isFree=true;
    }

    // debug: saare blocks print karo
    void printBlocks(){

        Header* cur=head;
        int idx=0;

        while(cur!=nullptr){

            long long int offset=reinterpret_cast<char*>(cur)-pool;

            cout<<"block "<<idx<<" | offset="<<offset<<" | size="<<cur->size<<" | "<<(cur->isFree?"FREE":"USED")<<endl;

            cur=cur->next;
            idx++;
        }

        cout<<"------"<<endl;
    }
};

int main(){

    FreeListAllocator alloc(256);

    void* a=alloc.allocate(20);
    void* b=alloc.allocate(50);
    void* c=alloc.allocate(10);

    cout<<"teen allocate ke baad:"<<endl;
    alloc.printBlocks();

    alloc.deallocate(b);
    cout<<"b free karne ke baad:"<<endl;
    alloc.printBlocks();

    void* d=alloc.allocate(8);
    cout<<"allocate(8) ke baad (b wala block split hoga):"<<endl;
    alloc.printBlocks();

    cout<<"a="<<a<<" b="<<b<<" c="<<c<<" d="<<d<<endl;

    return 0;
}