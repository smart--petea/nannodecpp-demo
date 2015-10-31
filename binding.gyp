{
    "targets": [
        {
            "target_name": "rainfall",
            "sources": [ "cpp/rainfall_node.cc", "cpp/rainfall.cc"],
            "cflags": ["-Wall", "-std=c++11"],
            "include_dirs" : [ "<!(node -e \"require('nan')\")" ]
        }
    ]
}
