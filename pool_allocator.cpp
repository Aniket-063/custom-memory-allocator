#include<bits/stdc++.h>
using namespace std;

class PoolAllocator{

    class FreeNode{
        public:
        FreeNode* next;
    };

    char* pool;
    FreeNode* head;
    long long int blockSize;
    long long int blockCount;

public:

    PoolAllocator(long long int size, long long int count){

        head=nullptr;
        blockCount=count;

        // block itna bada ho ki next pointer fit ho jaye
        if(size<sizeof(FreeNode)){
            size=sizeof(FreeNode);
        }

        // size ko 8 ke multiple tak round up karo
        blockSize=(size+7)&~7;

        // ek bada pool lo
        pool=new char[blockSize*blockCount];

        // saare blocks ko free list mein jodo (ulta loop, taaki block 0 sabse aage rahe)
        for(int i=blockCount-1; i>=0; i--){

            FreeNode* node=reinterpret_cast<FreeNode*>(pool+i*blockSize);
            node->next=head;
            head=node;
        }
    }

    ~PoolAllocator(){
        delete[] pool;
    }

    // copy band, warna double free ho jayega
    PoolAllocator(const PoolAllocator&)=delete;
    PoolAllocator& operator=(const PoolAllocator&)=delete;

    void* allocate(){

        // pool bhar gaya
        if(head==nullptr){
            return nullptr;
        }

        FreeNode* node=head;
        head=node->next;
        node->next=NULL;

        return node;
    }

    void deallocate(void* p){

        if(p==nullptr){
            return;
        }

        FreeNode* node=static_cast<FreeNode*>(p);
        node->next=head;
        head=node;
    }
};

int main(){

    PoolAllocator pool(32,4);   // 4 blocks, har ek 32 bytes

    void* a=pool.allocate();
    void* b=pool.allocate();

    cout<<"a = "<<a<<endl;
    cout<<"b = "<<b<<endl;

    pool.deallocate(a);

    void* c=pool.allocate();    // a wala block hi wapas milna chahiye
    cout<<"c = "<<c<<endl;

    if(c==a){
        cout<<"a ka block reuse hua!"<<endl;
    }

    // pool khatam hone ka test
    pool.allocate();
    pool.allocate();

    cout<<"5th alloc = "<<pool.allocate()<<" (nullptr aana chahiye)"<<endl;

    return 0;
}