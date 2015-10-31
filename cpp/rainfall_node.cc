#include <nan.h>
//#include "rainfall.h"
//
NAN_METHOD(AvgRainfall) {
    info.GetReturnValue().Set(Nan::New(0));
}


NAN_MODULE_INIT(init) {
    Nan::Set(target, Nan::New("avg_rainfall").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(AvgRainfall)->GetFunction());
}


NODE_MODULE(rainfall, init)
