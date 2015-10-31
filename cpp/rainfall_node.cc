#include <nan.h>
#include "rainfall.h"

sample unpack_sample(v8::Handle<v8::Object> sample_obj) {
    sample s;
    v8::Handle<v8::Value> date_Value = Nan::Get(sample_obj, Nan::New("date").ToLocalChecked()).ToLocalChecked();
    v8::Handle<v8::Value> rainfall_Value = Nan::Get(sample_obj, Nan::New("rainfall").ToLocalChecked()).ToLocalChecked();

    v8::String::Utf8Value utfValue(date_Value);
    s.date = std::string(*utfValue);
    s.rainfall = rainfall_Value->NumberValue();

    return s;
}

location unpack_location(Nan::NAN_METHOD_ARGS_TYPE info)
{
    location loc;
    v8::Handle<v8::Object> location_obj = v8::Handle<v8::Object>::Cast(info[0]);
    v8::Handle<v8::Value> lat_Value = Nan::Get(location_obj, Nan::New("latitutude").ToLocalChecked()).ToLocalChecked(); 
    v8::Handle<v8::Value> lon_Value = Nan::Get(location_obj, Nan::New("longitude").ToLocalChecked()).ToLocalChecked();

    loc.latitude = lat_Value->NumberValue();
    loc.longitude = lon_Value->NumberValue();

    v8::Handle<v8::Array> array = v8::Handle<v8::Array>::Cast(Nan::Get(location_obj, Nan::New("samples").ToLocalChecked()).ToLocalChecked());

    int sample_count = array->Length();

    for(int i = 0; i < sample_count; i++) {
        sample s = unpack_sample(v8::Handle<v8::Object>::Cast(Nan::Get(array, Nan::New(i)).ToLocalChecked()));
        loc.samples.push_back(s);
    }

    return loc;
}

NAN_METHOD(AvgRainfall) {
    location loc = unpack_location(info);
    double avg = avg_rainfall(loc);
    info.GetReturnValue().Set(Nan::New(avg));
}


NAN_MODULE_INIT(init) {
    Nan::Set(target, Nan::New("avg_rainfall").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(AvgRainfall)->GetFunction());
}


NODE_MODULE(rainfall, init)
