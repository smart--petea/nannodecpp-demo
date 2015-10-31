{
    "targets": [
        {
            "target_name": "rainfall",
            "sources": [ "cpp/rainfall_node.cc"],
            "include_dirs" : [ "<!(node -e \"require('nan')\")" ]
        }
    ]
}
