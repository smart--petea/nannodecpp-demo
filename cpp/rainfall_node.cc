#include <nan.h>
#include "rainfall.h"
#include <unistd.h>

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

    v8::Handle<v8::Array> array = Nan::Get(location_obj, Nan::New("samples").ToLocalChecked()).ToLocalChecked().As<v8::Array>();

    int sample_count = array->Length();

    for(int i = 0; i < sample_count; i++) {
        sample s = unpack_sample(Nan::Get(array, Nan::New(i)).ToLocalChecked().As<v8::Object>());
        loc.samples.push_back(s);
    }

    return loc;
}

location unpack_location(Nan::NAN_METHOD_ARGS_TYPE info)
{
    v8::Handle<v8::Object> location_obj = info[0].As<v8::Object>();
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

    v8::Local<v8::Array> input = info[0].As<v8::Array>();

    unsigned int num_locations = input->Length();
    for(unsigned int i = 0; i < num_locations; i++)
    {
        v8::Local<v8::Object> m = Nan::Get(input, Nan::New(i)).ToLocalChecked().As<v8::Object>();
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

struct Work {
    uv_work_t request;
    Nan::Persistent<v8::Function> callback;

    std::vector<location> *locations;
    std::vector<rain_result> *results;
};

static void WorkAsync(uv_work_t *req)
{
    Work *work = static_cast<Work *>(req->data);

    //This is the worker thread, lets bulid up the results
    //allocated results from the heap because we'll need
    //to accesss in the event loop laber to send back
    work->results = new std::vector<rain_result>();
    work->results->resize(work->locations->size());
    std::transform(work->locations->begin(),
            work->locations->end(),
            work->results->begin(),
            calc_rain_stats);

    //that wasn't really that long of an opeation,
    //so lets pretend it took longer ...

    sleep(3);
}

//called by libuv in event loop when async function completes
static void WorkAsyncComplete(uv_work_t *req, int status)
{
    Work *work = static_cast<Work *>(req->data);


    //the work has been done, and now we pack the results 
    //vector into a local array on the event-thread's stack
    v8::Local<v8::Array> result_list = Nan::New<v8::Array>();
    for(unsigned int i = 0; i < work->results->size(); i++)
    {
        v8::Local<v8::Object> result = Nan::New<v8::Object>();
        pack_rain_result(result, (*(work->results))[i]);
        result_list->Set(i, result);
    }

    //set up return arguments
    v8::Local<v8::Value> argv[] = { result_list };
    v8::Local<v8::Function> clbk = Nan::New(work->callback);

    Nan::MakeCallback(Nan::GetCurrentContext()->Global(), clbk, 1, argv);

    delete work;
}

NAN_METHOD(CalculateResultsAsync) {
    Work *work = new Work();
    work->request.data = work;

    //Locations is on the heap, accessible in the libuv threads
    work->locations = new std::vector<location>();
    v8::Local<v8::Array> input = info[0].As<v8::Array>();
    unsigned int num_locations = input->Length();
    for(unsigned int i = 0; i < num_locations; i++)
    {
        v8::Handle<v8::Object> m = input->Get(i).As<v8::Object>();
        work->locations->push_back(
            unpack_location(m)
        );
    }

    //store the callback from JS in the work package so we can invoke it later
    v8::Local<v8::Function> prst = info[1].As<v8::Function>();
    work->callback.Reset(prst);

    //kick of the worker thread
    uv_queue_work(uv_default_loop(), &work->request, WorkAsync, WorkAsyncComplete);

    info.GetReturnValue().Set(Nan::Undefined());
}


NAN_MODULE_INIT(init) {
    Nan::Set(target, Nan::New("avg_rainfall").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(AvgRainfall)->GetFunction());
    Nan::Set(target, Nan::New("data_rainfall").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(RainfallData)->GetFunction());
    Nan::Set(target, Nan::New("calculate_results").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CalculateResults)->GetFunction());
    Nan::Set(target, Nan::New("calculate_results_async").ToLocalChecked(), Nan::New<v8::FunctionTemplate>(CalculateResultsAsync)->GetFunction());
}


NODE_MODULE(rainfall, init)
