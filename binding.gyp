{
  "targets": [
  {
    "target_name": "jpeg-turbo-scaler",
      "sources": [ "jpeg.cc", "scaling.cc" ],
      "include_dirs": [ "/usr/local/opt/jpeg-turbo/include" ],
      "link_settings": {
        "libraries": [ "-L/usr/local/Cellar/jpeg-turbo/1.4.0/lib", "-lturbojpeg" ]
      },

  },
  {
    "target_name": "action_after_build",
    "type": "none",
    "dependencies": [ "jpeg-turbo-scaler" ],
    "copies": [ {
      "files": [ "<(PRODUCT_DIR)/jpeg-turbo-scaler.node" ],
      "destination": "."
    } ]
  }
  ]
}

