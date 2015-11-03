var rainfall = require("./build/Release/rainfall.node");

var makeup = function(max) {
    return Math.round(max * Math.random() * 100) / 100;
}

var print_rain_results = function (results) {
    //report the results from C++
    var i = 0;
    results.forEach(function(result) {
            console.log("Result for location " + i);
            console.log("------------------------------");
            console.log("\tLatitude:        " + locations[i].latitude.toFixed(2));
            console.log("\tLongitude:       " + locations[i].longitude.toFixed(2));
            console.log("\tMean Rainfall:   " + result.mean.toFixed(2) + " cm");
            console.log("\tMedian Rainfall: " + result.median.toFixed(2) + " cm");
            console.log("\tStandard dev.:   " + result.standard_deviation.toFixed(2));
            console.log("\tNumber Samples:  " + result.n);
            console.log();
            i++;
    });
}


//Build some dummy locations
var locations = [];
for(var i = 0; i < 10; i++) {
    var loc = {
        latitude: makeup(180),
        longitude: makeup(180),
        samples: [
            {date: "2015-07-20", rainfall: makeup(3) },
            {date: "2015-07-21", rainfall: makeup(3) },
            {date: "2015-07-22", rainfall: makeup(3) },
            {date: "2015-07-23", rainfall: makeup(3) },
        ]
    }
    locations.push(loc);
}

//Invoke the Addon
var results = rainfall.calculate_results(locations);
print_rain_results(results);

//pass a callback into the addon, and have the addon return immediately
rainfall.calculate_results_async(locations, print_rain_results);
