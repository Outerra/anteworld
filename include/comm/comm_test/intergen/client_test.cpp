
#include "ifc/client.h"


class handler_class : public client
{
public:

    void handler(int a, void* p) {
    }

    virtual void echo(int k) override {
        //call interface function giving it a member fn
        memfn_callback(&handler_class::handler);
    }
};

