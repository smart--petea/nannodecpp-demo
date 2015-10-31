#include <nan.h>
//#include "rainfall.h"

NAN_MODULE_INIT(init) {
    Nan::Set(target, Nan::New("ok").ToLocalChecked(), Nan::New("ok").ToLocalChecked());
}


NODE_MODULE(rainfall, init)
