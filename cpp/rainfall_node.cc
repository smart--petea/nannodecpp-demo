#include <nan.h>
#include "rainfall.h"

void pack_rain_result(v8::Local<v8::Object>& target, rain_result& result)
{
    Nan::Set(target, Nan::New("mean").ToLocalChecked(), Nan::New<v8::Number>(result.mean));
    Nan::Set(target, Nan::New("standard_deviation").ToLocalChecked(), Nan::New(result.standard_deviation));
    Nan::Set(target, Nan::New("median").ToLocalChecked(), Nan::New<v8::Number>(result.median));
    Nan::Set(target, Nan::New("n").ToLocalChecked(), Nan::New<v8::Integer>(result.n));
}

sample unpack_sample(v8::Handle<v8::Object> sample_obj) {
    sample s;
    v8::Handle<v8::Value> date_Value = Nan::Get(sample_obj, Nan::New("date").ToLocalChecked()).ToLocalChecked();
    v8::Handle<v8::Value> rainfall_Value = Nan::Get(sample_obj, Nan::New("rainfall").ToLocalChecked()).ToLocalChecked();

    v8::String::Utf8Value utfValue(date_Value);
    s.date = std::string(*utfValue);
    s.rainfall = rainfall_Value->NumberValue();

    return s;
}


location unpack_location(v8::Handle<v8::Object>& location_obj)
{
    location loc;
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

location unpack_location(Nan::NAN_METHOD_ARGS_TYPE info)
{
    v8::Handle<v8::Object> location_obj = v8::Handle<v8::Object>::Cast(info[0]);
    return unpack_location(location_obj);
}

NAN_METHOD(RainfallData) {
    location loc = unpack_location(info);
    rain_result result = calc_rain_stats(loc);

    v8::Local<v8::Object> obj = Nan::New<v8::Object>();

    pack_rain_result(obj, result);

    info.GetReturnValue().Set(obj);
}

NAN_METHOD(AvgRainfall) {
    location loc = unpack_location(info);
    double avg = avg_rainfall(loc);
    info.GetReturnValue().Set(Nan::New(avg));
}

NAN_METHOD(CalculateResults) {
    std::vector<location> locations; //we'll get this from Node.js
    std::vector<rain_result> results; //we'll build this in C++

    v8::Local<v8::Array> input = v8::Local<v8::Array>::Cast(info[0]);

    unsigned int num_locations = input->Length();
    for(unsigned int i = 0; i < num_locations; i++)
    {
        v8::Local<v8::Object> m = v8::Local<v8::Object>::Cast(Nan::Get(input, Nan::New(i)).ToLocalChecked());
        locations.push_back(
                unpack_location(m)
        );
    }

    results.resize(locations.size());
    std::transform(
            locations.begin(),
            locations.end(),
            results.begin(),
            calc_rain_stats
    );

    ////we'll populate this with the results
    v8::Local<v8::Array> result_list = Nan::New<v8::Array>();

    for(unsigned int i = 0; i < results.size(); i++)
    {
        v8::Local<v8::Object> result = Nan::New<v8::Object>();
        pack_rain_result(result, results[i]);
        Nan::Set(result_list, Nan::New(i), result);
    }

    info.GetReturnValue().Set(result_list);
}


NAN_MODULE_INIT(init) {
    Nan::Set(target, Nan::New("avg_rainfall").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(AvgRainfall)->GetFunction());
    Nan::Set(target, Nan::New("data_rainfall").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(RainfallData)->GetFunction());
    Nan::Set(target, Nan::New("calculate_results").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CalculateResults)->GetFunction());
}


NODE_MODULE(rainfall, init)
